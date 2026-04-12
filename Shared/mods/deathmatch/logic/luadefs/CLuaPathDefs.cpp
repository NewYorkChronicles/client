/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/mods/deathmatch/logic/luadefs/CLuaFileDefs.cpp
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"

#ifndef MTA_CLIENT
    // NOTE: Must be included before ILuaModuleManager.h which defines its own CChecksum type.
    #include "CChecksum.h"
#endif

#include "CLuaPathDefs.h"
#include "CScriptFile.h"
#include "CScriptArgReader.h"
#include <lua/CLuaFunctionParser.h>

void CLuaPathDefs::LoadFunctions()
{
    constexpr static const std::pair<const char*, lua_CFunction> functions[]{
        {"pathListDir", ArgumentParser<pathListDir>},
        {"pathIsFile", ArgumentParser<pathIsFile>},
        {"pathIsDirectory", ArgumentParser<pathIsDirectory>},
    };

    // Add functions
    for (const auto& [name, func] : functions)
        CLuaCFunctions::AddFunction(name, func);
}

void CLuaPathDefs::AddClass(lua_State* luaVM)
{
    lua_newclass(luaVM);

    lua_classfunction(luaVM, "listDir", "pathListDir");
    lua_classfunction(luaVM, "isFile", "pathIsFile");
    lua_classfunction(luaVM, "isDirectory", "pathIsDirectory");

    lua_registerclass(luaVM, "path");
}

std::optional<std::vector<std::string>> CLuaPathDefs::pathListDir(lua_State* luaVM, std::string path)
{
    CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
    if (!pLuaMain)
        return std::nullopt;

    std::string strAbsPath;

    CResource* pResource = pLuaMain->GetResource();
    if (!CResourceManager::ParseResourcePathInput(path, pResource, &strAbsPath))
    {
        m_pScriptDebugging->LogWarning(luaVM, "Cannot parse provided path: \"%s\"", path.c_str());
        return std::nullopt;
    }

    if (!DirectoryExists(strAbsPath))
    {
#ifdef MTA_CLIENT
        // Cache system flattens directories - synthesize listing from resource file list
        std::vector<std::string> result;
        std::set<std::string> entries;
        SString strPrefix = path;
        strPrefix = strPrefix.Replace("\\", "/");
        if (!strPrefix.empty() && strPrefix[strPrefix.length() - 1] != '/')
            strPrefix += "/";

        auto it = pResource->IterBeginResourceFiles();
        auto itEnd = pResource->IterEndResourceFiles();
        for (; it != itEnd; ++it)
        {
            SString strShort = (*it)->GetShortName();
            strShort = strShort.Replace("\\", "/");
            if (strShort.length() > strPrefix.length() && strShort.substr(0, strPrefix.length()) == strPrefix)
            {
                SString strRemainder = strShort.substr(strPrefix.length());
                auto slashPos = strRemainder.find('/');
                if (slashPos != std::string::npos)
                    entries.insert(std::string(strRemainder.substr(0, slashPos)));
                else
                    entries.insert(std::string(strRemainder));
            }
        }
        if (!entries.empty())
        {
            for (const auto& e : entries)
                result.push_back(e);
            return result;
        }
#endif
        m_pScriptDebugging->LogWarning(luaVM, "Directory \"%s\" doesn't exist!", path.c_str());
        return std::nullopt;
    }

    return SharedUtil::ListDir(strAbsPath.c_str());
}

bool CLuaPathDefs::pathIsFile(lua_State* luaVM, std::string path)
{
    CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
    if (!pLuaMain)
        return false;

    std::string strAbsPath;

    CResource* pResource = pLuaMain->GetResource();
    if (!CResourceManager::ParseResourcePathInput(path, pResource, &strAbsPath))
    {
        m_pScriptDebugging->LogWarning(luaVM, "Cannot parse provided path: \"%s\"", path.c_str());
        return false;
    }

    return SharedUtil::FileExists(strAbsPath);
}

bool CLuaPathDefs::pathIsDirectory(lua_State* luaVM, std::string path)
{
    CLuaMain* pLuaMain = m_pLuaManager->GetVirtualMachine(luaVM);
    if (!pLuaMain)
        return false;

    std::string strAbsPath;

    CResource* pResource = pLuaMain->GetResource();
    if (!CResourceManager::ParseResourcePathInput(path, pResource, &strAbsPath))
    {
        m_pScriptDebugging->LogWarning(luaVM, "Cannot parse provided path: \"%s\"", path.c_str());
        return false;
    }

    if (SharedUtil::DirectoryExists(strAbsPath.c_str()))
        return true;

#ifdef MTA_CLIENT
    // Cache system flattens directories - check if any resource files match this prefix
    SString strPrefix = path;
    strPrefix = strPrefix.Replace("\\", "/");
    if (!strPrefix.empty() && strPrefix[strPrefix.length() - 1] != '/')
        strPrefix += "/";

    auto it = pResource->IterBeginResourceFiles();
    auto itEnd = pResource->IterEndResourceFiles();
    for (; it != itEnd; ++it)
    {
        SString strShort = (*it)->GetShortName();
        strShort = strShort.Replace("\\", "/");
        if (strShort.length() > strPrefix.length() && strShort.substr(0, strPrefix.length()) == strPrefix)
            return true;
    }
#endif

    return false;
}
