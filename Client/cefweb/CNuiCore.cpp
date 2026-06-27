/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles Client
 *  FILE:        cefweb/CNuiCore.cpp
 *
 *****************************************************************************/
#include "StdInc.h"
#include "CNuiCore.h"
#include "CWebView.h"
#include <core/CAjaxResourceHandlerInterface.h>
#include <core/CRenderItemManagerInterface.h>

namespace
{
    constexpr const char* kRootUrl = "http://mta/__nui/root.html";

    SString JsEscape(const SString& s)
    {
        SString out;
        out.reserve(s.length() + 8);
        for (char c : s)
        {
            switch (c)
            {
                case '\\': out += "\\\\"; break;
                case '\'': out += "\\'";  break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '<':  out += "\\x3c"; break;
                default:   out += c;
            }
        }
        return out;
    }
}

CNuiCore& CNuiCore::Get()
{
    static CNuiCore s;
    return s;
}

void CNuiCore::Init()
{
    if (m_root) return;

    auto* pWebCore = g_pCore->GetWebCore();
    auto* pGfx     = g_pCore->GetGraphics();
    if (!pWebCore || !pWebCore->IsInitialised() || !pGfx) return;

    unsigned int w = pGfx->GetViewportWidth();
    unsigned int h = pGfx->GetViewportHeight();
    if (!w || !h) { w = 1280; h = 720; }

    m_rootItem = pGfx->GetRenderItemManager()->CreateWebBrowser(w, h);
    if (!m_rootItem) return;

    auto* pView = static_cast<CWebView*>(pWebCore->CreateWebView(w, h, true, m_rootItem, true));
    if (!pView) return;

    pView->SetWebBrowserEvents(this);
    pView->Initialise();
    m_root = pView;
}

void CNuiCore::Shutdown()
{
    if (!m_root) return;
    m_frames.clear();
    m_callbacks.clear();
    m_focused.clear();
    m_jsQueue.clear();
    m_visibleCount = 0;
    m_keepInput = false;
    m_rootReady = false;
    m_root->ClearWebBrowserEvents(this);
    m_root->CloseBrowser();
    m_root = nullptr;
    m_rootItem = nullptr;
}

void CNuiCore::exec(const SString& js)
{
    if (!m_root) return;
    if (!m_rootReady) { m_jsQueue += js; m_jsQueue += ';'; return; }
    m_root->ExecuteJavascript(js);
}

bool CNuiCore::CreateFrame(const SString& resource, const SString& url, int z, bool hidden)
{
    if (resource.empty() || url.empty() || m_frames.count(resource)) return false;
    m_frames[resource] = !hidden;
    if (!hidden) ++m_visibleCount;
    exec(SString("R.create('%s','%s',%d,%d)", JsEscape(resource).c_str(), JsEscape(url).c_str(), z, hidden ? 1 : 0));
    updateFrameRate();
    return true;
}

bool CNuiCore::ShowFrame(const SString& resource, bool visible)
{
    auto it = m_frames.find(resource);
    if (it == m_frames.end() || it->second == visible) return it != m_frames.end();

    it->second = visible;
    m_visibleCount += visible ? 1 : -1;
    exec(SString("R.show('%s',%d)", JsEscape(resource).c_str(), visible ? 1 : 0));

    if (!visible && m_focused == resource)
    {
        m_focused.clear();
        applyFocusState();
    }
    else
    {
        updateFrameRate();
    }
    return true;
}

bool CNuiCore::IsFrameVisible(const SString& resource) const
{
    auto it = m_frames.find(resource);
    return it != m_frames.end() && it->second;
}

bool CNuiCore::SendFrameMessage(const SString& resource, const SString& json)
{
    if (!m_frames.count(resource)) return false;
    exec(SString("R.send('%s','%s')", JsEscape(resource).c_str(), JsEscape(json).c_str()));
    return true;
}

bool CNuiCore::SetFocus(const SString& resource, bool enable, bool keepInput)
{
    if (enable)
    {
        if (!m_frames.count(resource)) return false;
        m_focused = resource;
        m_keepInput = keepInput;
    }
    else
    {
        if (!(resource.empty() || m_focused == resource))
            return true;
        m_focused.clear();
        m_keepInput = false;
    }
    applyFocusState();

    if (m_root)
        m_root->Focus(!m_focused.empty());
    return true;
}

void CNuiCore::applyFocusState()
{
    if (!m_focused.empty())
        exec(SString("R.focus('%s')", JsEscape(m_focused).c_str()));
    else
        exec("R.unfocus()");
    updateFrameRate();
}

// Idle-tick (5 Hz) when nothing is visible so iframes loading or navigating
// while hidden keep running JS — pausing CEF via WasHidden caused stale-frame
// flashes. 30 Hz ambient, 60 Hz when a frame is focused.
void CNuiCore::updateFrameRate()
{
    if (!m_root) return;
    auto browser = m_root->GetCefBrowser();
    if (!browser) return;
    auto host = browser->GetHost();
    if (!host) return;

    host->WasHidden(false);
    int rate = 5;
    if (m_visibleCount > 0) rate = m_focused.empty() ? 30 : 60;
    host->SetWindowlessFrameRate(rate);
}

void CNuiCore::Draw()
{
    if (!m_root || !m_rootItem || !m_rootReady || m_visibleCount == 0) return;
    m_root->UpdateTexture();
    auto* pGfx = g_pCore->GetGraphics();
    if (!pGfx) return;
    pGfx->DrawTexture(static_cast<CTextureItem*>(m_rootItem), 0, 0, 1.0f, 1.0f);
}

bool CNuiCore::ToggleDevTools(bool visible)
{
    return m_root && m_root->ToggleDevTools(visible);
}

bool CNuiCore::RegisterCallback(const SString& resource, const SString& type, NuiCallbackFn fn)
{
    if (resource.empty() || type.empty() || !fn) return false;
    m_callbacks[{resource, type}] = std::move(fn);
    return true;
}

bool CNuiCore::InvokeCallback(const SString& resource, const SString& type, const SString& body, SString& out)
{
    auto it = m_callbacks.find({resource, type});
    if (it == m_callbacks.end()) return false;
    out = it->second(body);
    return true;
}

void CNuiCore::OnResourceStop(const SString& resource)
{
    auto it = m_frames.find(resource);
    if (it != m_frames.end())
    {
        const bool wasFocused = (m_focused == resource);
        if (it->second) --m_visibleCount;
        if (wasFocused) { m_focused.clear(); m_keepInput = false; }
        m_frames.erase(it);
        exec(SString("R.destroy('%s')", JsEscape(resource).c_str()));
        if (wasFocused) applyFocusState();
        else            updateFrameRate();
    }

    for (auto cb = m_callbacks.begin(); cb != m_callbacks.end();)
        cb = (cb->first.resource == resource) ? m_callbacks.erase(cb) : std::next(cb);
}

void CNuiCore::Reset()
{
    if (!m_root) return;
    m_frames.clear();
    m_callbacks.clear();
    m_focused.clear();
    m_keepInput = false;
    m_visibleCount = 0;
    m_jsQueue.clear();
    m_rootReady = false;
    m_root->Focus(false);
    m_root->LoadURL(kRootUrl, false);
    updateFrameRate();
}

void CNuiCore::Events_OnCreated()
{
    if (m_root) m_root->LoadURL(kRootUrl, false);
}

void CNuiCore::Events_OnTriggerEvent(const SString& eventName, const std::vector<std::string>& args)
{
    if (eventName != "nuiInternal" || args.empty()) return;
    const std::string& kind = args[0];

    if (kind == "boot")
    {
        m_rootReady = true;
        if (!m_jsQueue.empty() && m_root)
        {
            m_root->ExecuteJavascript(m_jsQueue);
            m_jsQueue.clear();
        }
        return;
    }

    if (args.size() < 2) return;
    SStringX resource(args[1].c_str());

    if (kind == "msg" && m_listener)
    {
        SStringX ev  (args.size() > 2 ? args[2].c_str() : "");
        SStringX data(args.size() > 3 ? args[3].c_str() : "{}");
        m_listener->OnNuiMessage(resource, ev, data);
    }
    else if (kind == "focus")
    {
        SetFocus(resource, args.size() > 2 && args[2] == "1", args.size() > 3 && args[3] == "1");
    }
    else if (kind == "ready" && m_frames.count(resource) && m_listener)
    {
        m_listener->OnNuiReady(resource);
    }
}

void CNuiCore::Events_OnAjaxRequest(CAjaxResourceHandlerInterface* h, const SString&)
{
    if (h) h->SetResponse("");
}

void CNuiCore::Events_OnConsoleMessage(const std::string& msg, const std::string& src, int line, std::int16_t lvl)
{
    SString resource = "nui";
    for (const char* p : {"http://mta/", "https://nyc-nui-"})
    {
        const size_t pLen = strlen(p);
        if (src.rfind(p, 0) != 0) continue;
        const size_t slash = src.find('/', pLen);
        resource = src.substr(pLen, (slash == std::string::npos ? src.size() : slash) - pLen).c_str();
        break;
    }

    const char* label = (lvl >= 3) ? "ERR" : (lvl >= 2) ? "WARN" : "LOG";
    g_pCore->GetConsole()->Printf("[nui:%s] [%s] %s (%s:%d)", resource.c_str(), label, msg.c_str(), src.c_str(), line);

    if (m_listener)
        m_listener->OnNuiConsole(resource, (int)lvl, SStringX(msg.c_str()), SStringX(src.c_str()), line);
}

const char* CNuiCore::GetRootHtml()
{
    static const char kHtml[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><title>nui</title>
<style>
html,body{margin:0;padding:0;width:100vw;height:100vh;overflow:hidden;background:transparent;font-family:system-ui,Arial,sans-serif;color:#fff}
#layers{position:fixed;inset:0;pointer-events:none}
iframe.nui{position:fixed;inset:0;width:100vw;height:100vh;border:0;background:transparent;contain:layout paint style;pointer-events:none}
iframe.nui.focused{pointer-events:auto}
iframe.nui.hidden{visibility:hidden;pointer-events:none}
</style></head><body>
<div id="layers"></div>
<script>
(function(){
'use strict';
try { localStorage.clear(); sessionStorage.clear(); } catch(_){}
var $layers = document.getElementById('layers');
var frames = Object.create(null);

function emit(kind, res, ev, data){
  try { mta.triggerEvent('nuiInternal', String(kind), String(res||''), String(ev||''), data||''); } catch(_){}
}

var focusedRes = '';
function applyFocus(res){
  var f = frames[res]; if (!f) return;
  try { f.el.contentWindow.focus(); } catch(_){}
}

window.addEventListener('message', function(e){
  var src = e.source;
  for (var res in frames){
    if (frames[res].el.contentWindow !== src) continue;
    var d = e.data || {};
    if (d._ctl === 'focus') return emit('focus', res, d.enable?'1':'0', d.keepInput?'1':'0');
    if (d._ctl === 'ready'){
      if (focusedRes === res) applyFocus(res);
      return emit('ready', res, '', '');
    }
    var ev = d.event || '';
    var payload = ''; try { payload = JSON.stringify(d.data !== undefined ? d.data : d); } catch(_){ payload = '{}'; }
    return emit('msg', res, ev, payload);
  }
}, false);

window.R = {
  create: function(res, url, z, hidden){
    if (frames[res]) return;
    var fullUrl = url + (url.indexOf('#') >= 0 ? '&' : '#') + '__nuires=' + encodeURIComponent(res);
    var f = document.createElement('iframe');
    f.className = 'nui' + (hidden ? ' hidden' : '');
    f.name = 'nui:' + res;
    f.style.zIndex = z|0;
    f.src = fullUrl;
    $layers.appendChild(f);
    frames[res] = { el: f };
  },
  destroy: function(res){
    var f = frames[res]; if (!f) return;
    if (focusedRes === res) focusedRes = '';
    if (f.el.parentNode) f.el.parentNode.removeChild(f.el);
    delete frames[res];
  },
  show: function(res, v){
    var f = frames[res]; if (!f) return;
    f.el.classList.toggle('hidden', !v);
    if (v && focusedRes === res) applyFocus(res);
  },
  send: function(res, jsonStr){
    var f = frames[res]; if (!f || !f.el.contentWindow) return;
    var data = {}; try { data = JSON.parse(jsonStr); } catch(_){}
    try { f.el.contentWindow.postMessage(data, '*'); } catch(_){}
  },
  focus: function(res){
    if (focusedRes === res){ applyFocus(res); return; }
    var prev = focusedRes; focusedRes = res;
    if (prev && frames[prev]){
      frames[prev].el.classList.remove('focused');
      try { frames[prev].el.contentWindow.blur(); } catch(_){}
    }
    var f = frames[res]; if (!f) return;
    f.el.classList.add('focused');
    applyFocus(res);
  },
  unfocus: function(){
    var prev = focusedRes; focusedRes = '';
    if (!prev) return;
    var f = frames[prev]; if (!f) return;
    f.el.classList.remove('focused');
    try { f.el.contentWindow.blur(); } catch(_){}
  }
};

try { mta.triggerEvent('nuiInternal','boot','','',''); } catch(_){}
})();
</script></body></html>
)HTML";
    return kHtml;
}
