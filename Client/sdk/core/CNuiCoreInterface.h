/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles Client
 *  FILE:        sdk/core/CNuiCoreInterface.h
 *  PURPOSE:     NUI subsystem interface (cefweb → deathmatch boundary)
 *
 *****************************************************************************/
#pragma once
#include <functional>

class SString;

namespace SharedUtil { class CBuffer; }

using NuiCallbackFn = std::function<SString(const SString& body)>;

class INuiEventListener
{
public:
    virtual ~INuiEventListener() = default;
    virtual void OnNuiMessage(const SString& resource, const SString& event, const SString& data) = 0;
    virtual void OnNuiReady(const SString& resource) = 0;
    virtual void OnNuiConsole(const SString& resource, int level, const SString& msg, const SString& src, int line) = 0;
    virtual bool LoadResourceFile(const SString& path, SharedUtil::CBuffer& outData) = 0;
};

class CNuiCoreInterface
{
public:
    virtual ~CNuiCoreInterface() = default;

    virtual bool CreateFrame(const SString& resource, const SString& url, int z, bool hidden) = 0;
    virtual bool HasFrame(const SString& resource) const = 0;
    virtual bool ShowFrame(const SString& resource, bool visible) = 0;
    virtual bool IsFrameVisible(const SString& resource) const = 0;
    virtual bool SendFrameMessage(const SString& resource, const SString& json) = 0;

    virtual bool SetFocus(const SString& resource, bool enable, bool keepInput) = 0;
    virtual bool IsFocused() const = 0;
    virtual bool IsKeepInput() const = 0;

    virtual bool RegisterCallback(const SString& resource, const SString& type, NuiCallbackFn fn) = 0;

    virtual void OnResourceStop(const SString& resource) = 0;
    virtual void Reset() = 0;
    virtual bool ToggleDevTools(bool visible) = 0;
    virtual void SetEventListener(INuiEventListener* listener) = 0;
    virtual void Draw() = 0;
};
