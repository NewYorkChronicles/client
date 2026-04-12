/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CTransferBox.cpp
 *  PURPOSE:     Transfer box - DX rendered download notification
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#include <StdInc.h>

CTransferBox::CTransferBox(TransferBoxType transferType)
{
    switch (transferType)
    {
        case TransferBoxType::MAP_DOWNLOAD:
            m_titleProgressPrefix = _("Map download progress:");
            break;
        default:
            m_titleProgressPrefix = _("Download Progress:");
    }

    m_visible.set(TB_VISIBILITY_CLIENT_SCRIPT);
    m_visible.set(TB_VISIBILITY_SERVER_SCRIPT);

    g_pCore->GetCVars()->Get("always_show_transferbox", m_alwaysVisible);
}

void CTransferBox::Show()
{
    m_visible.set(TB_VISIBILITY_MTA);
    UpdateDisplay();
}

void CTransferBox::Hide()
{
    m_visible.reset(TB_VISIBILITY_MTA);
    m_downloadTotalSize = 0;
    g_pCore->GetGraphics()->ClearDownloadProgress();
}

void CTransferBox::SetDownloadProgress(uint64_t downloadedSizeTotal)
{
    if (m_downloadTotalSize == 0)
        return;

    SString current = GetDataUnit(downloadedSizeTotal);
    SString total = GetDataUnit(m_downloadTotalSize);
    SString message = SString("%s %s / %s", m_titleProgressPrefix.c_str(), current.c_str(), total.c_str());
    float progress = static_cast<float>(static_cast<double>(downloadedSizeTotal) / m_downloadTotalSize);

    g_pCore->GetGraphics()->SetDownloadProgress(message, progress);
}

void CTransferBox::DoPulse()
{
}

bool CTransferBox::SetClientVisibility(bool visible)
{
    if (m_visible[TB_VISIBILITY_CLIENT_SCRIPT] == visible)
        return false;

    m_visible.set(TB_VISIBILITY_CLIENT_SCRIPT, visible);
    UpdateDisplay();
    return true;
}

bool CTransferBox::SetServerVisibility(bool visible)
{
    if (m_visible[TB_VISIBILITY_SERVER_SCRIPT] == visible)
        return false;

    m_visible.set(TB_VISIBILITY_SERVER_SCRIPT, visible);
    UpdateDisplay();
    return true;
}

bool CTransferBox::SetAlwaysVisible(bool visible)
{
    if (m_alwaysVisible == visible)
        return false;

    m_alwaysVisible = visible;
    UpdateDisplay();
    return true;
}

void CTransferBox::UpdateDisplay()
{
    bool visible = m_visible.all() || (m_visible[TB_VISIBILITY_MTA] && m_alwaysVisible);
    if (!visible)
        g_pCore->GetGraphics()->ClearDownloadProgress();
}
