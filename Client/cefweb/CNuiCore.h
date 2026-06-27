/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles Client
 *  FILE:        cefweb/CNuiCore.h
 *  PURPOSE:     Single-browser NUI manager (FiveM-style UI layer)
 *
 *****************************************************************************/
#pragma once

#include <core/CWebBrowserEventsInterface.h>
#include <core/CNuiCoreInterface.h>
#include <unordered_map>
#include <vector>
#include <SString.h>

class CWebView;
class CWebBrowserItem;

struct SNuiCallbackKey
{
    SString resource, type;
    bool    operator==(const SNuiCallbackKey& o) const { return resource == o.resource && type == o.type; }
};

struct SNuiCallbackKeyHash
{
    size_t operator()(const SNuiCallbackKey& k) const noexcept
    {
        return std::hash<std::string>{}(std::string(k.resource) + '\x1f' + std::string(k.type));
    }
};

class CNuiCore : public CNuiCoreInterface, public CWebBrowserEventsInterface
{
public:
    static CNuiCore& Get();

    void Init();
    void Shutdown();
    bool IsRootReady() const noexcept { return m_rootReady; }
    CWebView* GetRoot() const noexcept { return m_root; }

    bool CreateFrame(const SString& resource, const SString& url, int z, bool hidden) override;
    bool HasFrame(const SString& resource) const override { return m_frames.count(resource) > 0; }
    bool ShowFrame(const SString& resource, bool visible) override;
    bool IsFrameVisible(const SString& resource) const override;
    bool SendFrameMessage(const SString& resource, const SString& json) override;
    bool SetFocus(const SString& resource, bool enable, bool keepInput) override;
    bool IsFocused() const override { return !m_focused.empty(); }
    bool IsKeepInput() const override { return m_keepInput; }
    bool RegisterCallback(const SString& resource, const SString& type, NuiCallbackFn fn) override;
    void OnResourceStop(const SString& resource) override;
    void Reset() override;
    bool ToggleDevTools(bool visible) override;
    void SetEventListener(INuiEventListener* listener) override { m_listener = listener; }
    void Draw() override;

    bool InvokeCallback(const SString& resource, const SString& type, const SString& body, SString& out);

    static const char* GetRootHtml();

    void Events_OnCreated() override;
    void Events_OnLoadingStart(const SString&, bool) override {}
    void Events_OnDocumentReady(const SString&) override {}
    void Events_OnLoadingFailed(const SString&, int, const SString&) override {}
    void Events_OnNavigate(const SString&, bool, bool) override {}
    void Events_OnPopup(const SString&, const SString&) override {}
    void Events_OnChangeCursor(unsigned char) override {}
    void Events_OnTriggerEvent(const SString& eventName, const std::vector<std::string>& args) override;
    void Events_OnTooltip(const SString&) override {}
    void Events_OnInputFocusChanged(bool) override {}
    bool Events_OnResourcePathCheck(SString&) override { return true; }
    bool Events_OnResourceFileCheck(const SString& path, CBuffer& out) override
    {
        return m_listener && m_listener->LoadResourceFile(path, out);
    }
    void Events_OnResourceBlocked(const SString&, const SString&, unsigned char) override {}
    void Events_OnAjaxRequest(class CAjaxResourceHandlerInterface* h, const SString&) override;
    void Events_OnConsoleMessage(const std::string& msg, const std::string& src, int line, std::int16_t lvl) override;

private:
    CNuiCore() = default;
    ~CNuiCore() = default;
    CNuiCore(const CNuiCore&) = delete;
    CNuiCore& operator=(const CNuiCore&) = delete;

    void exec(const SString& js);
    void applyFocusState();
    void updateFrameRate();

    CWebView*          m_root = nullptr;
    CWebBrowserItem*   m_rootItem = nullptr;
    INuiEventListener* m_listener = nullptr;
    bool               m_rootReady = false;
    SString            m_focused;
    bool               m_keepInput = false;
    int                m_visibleCount = 0;

    std::unordered_map<SString, bool>                                       m_frames;        // resource → visible
    std::unordered_map<SNuiCallbackKey, NuiCallbackFn, SNuiCallbackKeyHash> m_callbacks;
    SString                                                                 m_jsQueue;
};
