/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles Client
 *  FILE:        mods/deathmatch/logic/luadefs/CLuaNuiDefs.cpp
 *
 *****************************************************************************/
#include "StdInc.h"
#include "CLuaNuiDefs.h"
#include <core/CNuiCoreInterface.h>

namespace
{
    CNuiCoreInterface* Nui()
    {
        auto* wc = g_pCore->GetWebCore();
        return wc ? wc->GetNuiCore() : nullptr;
    }

    SString JsonUnwrap(const std::string& wrapped)
    {
        if (wrapped.size() >= 2 && wrapped.front() == '[' && wrapped.back() == ']')
            return SStringX(wrapped.c_str() + 1, (uint)wrapped.size() - 2);
        return SStringX(wrapped.c_str());
    }

    SString TableToJson(lua_State* L, int idx)
    {
        CLuaArguments args;
        args.ReadArgument(L, idx);
        std::string out;
        args.WriteToJSONString(out, false);
        return JsonUnwrap(out);
    }
}

void CLuaNuiDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> fns[]{
        {"showNuiFrame",        ShowNuiFrame},
        {"hideNuiFrame",        HideNuiFrame},
        {"sendNuiMessage",      SendNuiMessage},
        {"registerNuiCallback", RegisterNuiCallback},
        {"setNuiFocus",         SetNuiFocus},
        {"isNuiFocused",        IsNuiFocused},
        {"toggleNuiDevTools",   ToggleNuiDevTools},
    };
    for (const auto& [n, f] : fns) CLuaCFunctions::AddFunction(n, f);
}

SString CLuaNuiDefs::CallerResource(lua_State* L)
{
    CLuaMain*  pLuaMain = m_pLuaManager->GetVirtualMachine(L);
    CResource* pResource = pLuaMain ? pLuaMain->GetResource() : nullptr;
    return pResource ? pResource->GetName() : SString();
}

int CLuaNuiDefs::ShowNuiFrame(lua_State* L)
{
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->ShowFrame(CallerResource(L), true));
    return 1;
}

int CLuaNuiDefs::HideNuiFrame(lua_State* L)
{
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->ShowFrame(CallerResource(L), false));
    return 1;
}

int CLuaNuiDefs::SendNuiMessage(lua_State* L)
{
    if (!lua_istable(L, 1)) return luaL_error(L, "sendNuiMessage expects a table");
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->SendFrameMessage(CallerResource(L), TableToJson(L, 1)));
    return 1;
}

int CLuaNuiDefs::RegisterNuiCallback(lua_State* L)
{
    SString         type;
    CLuaFunctionRef fn;

    CScriptArgReader argStream(L);
    argStream.ReadString(type);
    argStream.ReadFunction(fn);
    argStream.ReadFunctionComplete();
    if (argStream.HasErrors()) return luaL_error(L, argStream.GetFullErrorMessage());

    CLuaMain*  pLuaMain = m_pLuaManager->GetVirtualMachine(L);
    CResource* pResource = pLuaMain ? pLuaMain->GetResource() : nullptr;
    auto*      nui = Nui();
    if (!pResource || !nui || !VERIFY_FUNCTION(fn)) { lua_pushboolean(L, false); return 1; }

    CResourceManager* pRM = m_pResourceManager;
    auto              netId = pResource->GetNetID();

    bool ok = nui->RegisterCallback(
        pResource->GetName(), type,
        [=](const SString& body) -> SString
        {
            if (!pRM->Exists(pResource) || pResource->GetNetID() != netId || !VERIFY_FUNCTION(fn)) return "";

            CLuaArguments args;
            args.PushString(body);

            CLuaArguments result;
            args.Call(pLuaMain, fn, &result);
            if (result.IsEmpty()) return "";

            CLuaArgument* rv = *result.begin();
            if (rv->GetType() == LUA_TSTRING) return rv->GetString();
            if (rv->GetType() == LUA_TTABLE)
            {
                CLuaArguments one; one.PushArgument(*rv);
                std::string out;
                one.WriteToJSONString(out, false);
                return JsonUnwrap(out);
            }
            return "";
        });

    lua_pushboolean(L, ok);
    return 1;
}

int CLuaNuiDefs::SetNuiFocus(lua_State* L)
{
    bool enable, keepInput;
    CScriptArgReader argStream(L);
    argStream.ReadBool(enable);
    argStream.ReadBool(keepInput, false);
    if (argStream.HasErrors()) return luaL_error(L, argStream.GetFullErrorMessage());
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->SetFocus(CallerResource(L), enable, keepInput));
    return 1;
}

int CLuaNuiDefs::IsNuiFocused(lua_State* L)
{
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->IsFocused());
    return 1;
}

int CLuaNuiDefs::ToggleNuiDevTools(lua_State* L)
{
    bool visible;
    CScriptArgReader argStream(L);
    argStream.ReadBool(visible, true);
    if (argStream.HasErrors()) return luaL_error(L, argStream.GetFullErrorMessage());
    auto* nui = Nui();
    lua_pushboolean(L, nui && nui->ToggleDevTools(visible));
    return 1;
}
