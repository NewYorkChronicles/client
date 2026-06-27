/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        ceflauncher/CCefApp.h
 *  PURPOSE:     CefApp implementation
 *
 *****************************************************************************/
#include <cef3/cef/include/cef_app.h>
#include <string>
#include <sstream>
#include "V8Helpers.h"
using V8Helpers::CV8Handler;

class CCefApp : public CefApp, public CefRenderProcessHandler
{
public:
    CCefApp() {}
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; };

    // https://magpcss.org/ceforum/apidocs3/projects/(default)/CefRenderProcessHandler.html#OnFocusedNodeChanged(CefRefPtr%3CCefBrowser%3E,CefRefPtr%3CCefFrame%3E,CefRefPtr%3CCefDOMNode%3E)
    virtual void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) override
    {
        if (m_bHasInputFocus)
        {
            if (node)
                return;

            // Tell MTA that we lost input focus
            auto message = CefProcessMessage::Create("InputFocus");
            message->GetArgumentList()->SetBool(0, false);
            browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);

            // Set variable to ensure that the event does not trigger twice
            m_bHasInputFocus = false;
            return;
        }
        else
        {
            if (!node)
                return;

#ifdef MTA_MAETRO
            if (node->GetType() == CefDOMNode::Type::DOM_NODE_TYPE_ELEMENT && !node->GetFormControlElementType().empty())
#else
            if (node->GetType() == CefDOMNode::Type::DOM_NODE_TYPE_ELEMENT &&
                node->GetFormControlElementType() != CefDOMNode::FormControlType::DOM_FORM_CONTROL_TYPE_UNSUPPORTED)
#endif
            {
                auto message = CefProcessMessage::Create("InputFocus");
                message->GetArgumentList()->SetBool(0, true);
                browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);

                // Set variable to ensure that the event does not trigger twice
                m_bHasInputFocus = true;
            }
        }
    }

    // https://magpcss.org/ceforum/apidocs3/projects/(default)/CefRenderProcessHandler.html#OnContextCreated(CefRefPtr%3CCefBrowser%3E,CefRefPtr%3CCefFrame%3E,CefRefPtr%3CCefV8Context%3E)
    // //
    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override
    {
        CefRefPtr<CefV8Value> globalObject = context->GetGlobal();
        CefRefPtr<CV8Handler> handler = new CV8Handler(frame);

        CefRefPtr<CefV8Value> mtaObject = CefV8Value::CreateObject(nullptr, nullptr);
        V8Helpers::BindV8Function(handler, mtaObject, "triggerEvent", Javascript_triggerEvent);
        globalObject->SetValue("mta", mtaObject, V8_PROPERTY_ATTRIBUTE_NONE);

        InstallNuiGlobal(frame, globalObject);
    }

    // Injects window.nui into every V8 context. Resource name is resolved
    // lazily inside JS from document.URL — needed because same-origin iframe
    // navigation reuses the V8 context, so the URL captured here would be
    // stale (or empty) by the time the loaded page actually calls nui.fetch.
    static void InstallNuiGlobal(CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Value>)
    {
        static const char script[] =
            "(function(){"
            "var t=window.parent!==window?window.parent:window;"
            "var H={};"
            "function getRn(){"
            "var u=(document.URL||'').split('#')[0];"
            "var m=u.match(/^https?:\\/\\/(?:mta\\/([^\\/]+)|nyc-nui-([^\\/]+))/);"
            "return m?(m[1]||m[2]):'';"
            "}"
            "function tag(a){a=[].slice.call(a);a.unshift('['+getRn()+']');return a;}"
            "var nui={"
            "get resourceName(){return getRn();},"
            "emit:function(e,d){try{t.postMessage({event:e,data:d},'*');}catch(_){}} ,"
            "setFocus:function(e,k){try{t.postMessage({_ctl:'focus',enable:!!e,keepInput:!!k},'*');}catch(_){}} ,"
            "on:function(ev,fn){if(typeof fn!=='function')return;(H[ev]=H[ev]||[]).push(fn);} ,"
            "off:function(ev,fn){var a=H[ev];if(!a)return;if(!fn){delete H[ev];return;}var i=a.indexOf(fn);if(i>=0)a.splice(i,1);} ,"
            "log:function(){console.log.apply(console,tag(arguments));} ,"
            "warn:function(){console.warn.apply(console,tag(arguments));} ,"
            "error:function(){console.error.apply(console,tag(arguments));} ,"
            "fetch:function(ty,b){"
            "var p=(typeof b==='string')?b:JSON.stringify(b||{});"
            "return fetch('http://mta/__nuirpc/'+encodeURIComponent(getRn())+'/'+encodeURIComponent(ty),"
            "{method:'POST',body:p,headers:{'Content-Type':'application/json'}})"
            ".then(function(r){return r.ok?r.text().then(function(x){try{return JSON.parse(x);}catch(_){return x;}}):Promise.reject(r.status);});}"
            "};"
            "window.nui=nui;"
            "window.addEventListener('message',function(e){"
            "var d=e&&e.data;if(!d||typeof d!=='object')return;"
            "var ev=d.event||d.action;if(!ev)return;"
            "var arr=H[ev];if(!arr)return;"
            "var payload=(d.data!==undefined)?d.data:d;"
            "for(var i=0;i<arr.length;i++){try{arr[i](payload);}catch(_){}}"
            "},false);"
            "var fireReady=function(){try{t.postMessage({_ctl:'ready'},'*');}catch(_){}};"
            "if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',fireReady);"
            "else fireReady();"
            "})();";

        frame->ExecuteJavaScript(script, frame->GetURL(), 0);
    }

    static void Javascript_triggerEvent(CefRefPtr<CefFrame> frame, const CefV8ValueList& arguments)
    {
        if (arguments.size() == 0)
            return;

        CefRefPtr<CefProcessMessage> message = V8Helpers::SerialiseV8Arguments("TriggerLuaEvent", arguments);
        frame->GetBrowser()->GetMainFrame()->SendProcessMessage(PID_BROWSER, message);
    }

public:
    IMPLEMENT_REFCOUNTING(CCefApp);

private:
    bool m_bHasInputFocus = false;
};
