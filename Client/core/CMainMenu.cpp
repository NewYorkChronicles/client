/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CMainMenu.cpp
 *  PURPOSE:     2D Main menu graphical user interface
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include <game/CGame.h>
#include "CNewsBrowser.h"
#include "CLanguageSelector.h"
#include "CDiscordRichPresence.h"

#define NATIVE_RES_X 1280.0f
#define NATIVE_RES_Y 1024.0f

#define NATIVE_BG_X 1280.0f
#define NATIVE_BG_Y 649.0f

#define NATIVE_LOGO_X 1058.0f
#define NATIVE_LOGO_Y 540.0f

#define CORE_MTA_MENUITEMS_START_X 0.168

#define CORE_MTA_BG_MAX_ALPHA    0.80f
#define CORE_MTA_BG_INGAME_ALPHA 0.70f
#define CORE_MTA_FADER           0.05f  // 1/20
#define CORE_MTA_FADER_CREDITS   0.01f

#define CORE_MTA_HOVER_SCALE  1.0f
#define CORE_MTA_NORMAL_SCALE 0.6f
#define CORE_MTA_HOVER_ALPHA  1.0f
#define CORE_MTA_NORMAL_ALPHA 0.6f

#define CORE_MTA_HIDDEN_ALPHA   0.0f
#define CORE_MTA_DISABLED_ALPHA 0.4f
#define CORE_MTA_ENABLED_ALPHA  1.0f

#define CORE_MTA_ANIMATION_TIME_IN  200
#define CORE_MTA_ANIMATION_TIME_OUT 100
#define CORE_MTA_MOVE_ANIM_TIME     600

#define CORE_MTA_STATIC_BG "cgui\\images\\background.png"
#define CORE_MTA_LOGO      "cgui\\images\\background_logo.png"
#define CORE_MTA_FILLER    "cgui\\images\\mta_filler.png"
#define CORE_MTA_VERSION   "cgui\\images\\version.png"

static const SColor headlineColors[] = {SColorRGBA(233, 234, 106, 255), SColorRGBA(233 / 6 * 4, 234 / 6 * 4, 106 / 6 * 4, 255),
                                        SColorRGBA(233 / 7 * 3, 234 / 7 * 3, 106 / 7 * 3, 255)};

// Improve alignment with magical mystery values!
static const int       BODGE_FACTOR_1 = -5;
static const int       BODGE_FACTOR_2 = 5;
static const int       BODGE_FACTOR_3 = -5;
static const int       BODGE_FACTOR_4 = 5;
static const CVector2D BODGE_FACTOR_5(0, -50);
static const CVector2D BODGE_FACTOR_6(0, 100);

CMainMenu::CMainMenu(CGUI* pManager)
{
    m_pNewsBrowser = new CNewsBrowser();

    ulPreviousTick = GetTickCount32();
    m_pHoveredItem = NULL;
    m_iMoveStartPos = 0;

    // Initialize
    m_pManager = pManager;
    m_bIsVisible = true;
    m_bIsFullyVisible = false;
    m_bIsIngame = true;
    //    m_bIsInSubWindow = false;
    m_fFader = 0;
    m_ucFade = FADE_INVISIBLE;
    m_bCursorAlphaReset = false;

    // Adjust window size to resolution
    CVector2D ScreenSize = m_pManager->GetResolution();
    m_ScreenSize = ScreenSize;

    int iBackgroundX = 0;
    int iBackgroundY = 0;
    int iBackgroundSizeX = ScreenSize.fX;
    int iBackgroundSizeY;

    // First let's work out our x and y offsets
    if (ScreenSize.fX > ScreenSize.fY)  // If the monitor is a normal landscape one
    {
        float iRatioSizeY = ScreenSize.fY / NATIVE_RES_Y;
        m_iMenuSizeX = NATIVE_RES_X * iRatioSizeY;
        m_iMenuSizeY = ScreenSize.fY;
        m_iXOff = (ScreenSize.fX - m_iMenuSizeX) * 0.5f;
        m_iYOff = 0;

        float iRatioSizeX = ScreenSize.fX / NATIVE_RES_X;
        iBackgroundSizeX = ScreenSize.fX;
        iBackgroundSizeY = NATIVE_BG_Y * iRatioSizeX;
    }
    else  // Otherwise our monitor is in a portrait resolution, so we cant fill the background by y
    {
        float iRatioSizeX = ScreenSize.fX / NATIVE_RES_X;
        m_iMenuSizeY = NATIVE_RES_Y * iRatioSizeX;
        m_iMenuSizeX = ScreenSize.fX;
        m_iXOff = 0;
        m_iYOff = (ScreenSize.fY - m_iMenuSizeY) * 0.5f;

        iBackgroundY = m_iYOff;
        iBackgroundSizeX = m_iMenuSizeX;
        iBackgroundSizeY = NATIVE_BG_Y * iRatioSizeX;
    }
    m_pFiller = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage());
    m_pFiller->LoadFromFile(CORE_MTA_FILLER);
    m_pFiller->SetVisible(false);
    m_pFiller->SetSize(CVector2D(0, 0), false);

    m_pBackground = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage());
    m_pBackground->LoadFromFile(CORE_MTA_FILLER);
    m_pBackground->SetVisible(false);
    m_pBackground->SetSize(CVector2D(0, 0), false);

    m_pFiller2 = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage());
    m_pFiller2->LoadFromFile(CORE_MTA_FILLER);
    m_pFiller2->SetVisible(false);
    m_pFiller2->SetSize(CVector2D(0, 0), false);

    m_pCanvas = reinterpret_cast<CGUIScrollPane*>(pManager->CreateScrollPane());
    m_pCanvas->SetProperty("ContentPaneAutoSized", "False");
    m_pCanvas->SetPosition(CVector2D(m_iXOff, m_iYOff), false);
    m_pCanvas->SetSize(CVector2D(m_iMenuSizeX, m_iMenuSizeY), false);
    m_pCanvas->SetZOrderingEnabled(false);
    m_pCanvas->SetAlwaysOnTop(true);
    m_pCanvas->MoveToBack();
    m_pCanvas->SetVisible(false);

    // Create our MTA logo
    CVector2D logoSize = CVector2D((NATIVE_LOGO_X / NATIVE_RES_X) * m_iMenuSizeX, (NATIVE_LOGO_Y / NATIVE_RES_Y) * m_iMenuSizeY);
    m_pLogo = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage(m_pCanvas));
    m_pLogo->LoadFromFile(CORE_MTA_FILLER);
    m_pLogo->SetVisible(false);
    m_pLogo->SetSize(CVector2D(0, 0), false);
    m_pLogo->SetZOrderingEnabled(false);

    m_pVersion = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage());
    m_pVersion->SetParent(m_pCanvas);
    m_pVersion->SetVisible(false);

    float fBase = 0.613f;
    float fGap = 0.043f;
    m_pDisconnect = nullptr;

    m_menuItems.push_back(CreateItem(MENU_ITEM_SETTINGS, "menu_settings.png", CVector2D(0.168f, fBase + fGap * 0)));
    m_menuItems.push_back(CreateItem(MENU_ITEM_QUIT, "menu_quit.png", CVector2D(0.168f, fBase + fGap * 1)));

    float fFirstItemSize = m_menuItems.front()->image->GetSize(false).fY;
    m_iFirstItemCentre = (m_menuItems.front()->image)->GetPosition().fY + fFirstItemSize * 0.5f;
    m_iSecondItemCentre = m_iFirstItemCentre + (int)(fGap * m_iMenuSizeY);

    // Store some mouse over bounding box positions
    m_menuAX = (0.168f * m_iMenuSizeX) + m_iXOff;                                                            // Left side of the items
    m_menuAY = m_iFirstItemCentre - fFirstItemSize * (CORE_MTA_HOVER_SCALE / CORE_MTA_NORMAL_SCALE) * 0.5f;  // Top side of the items
    m_menuBX = m_menuAX + ((390 / NATIVE_RES_X) * m_iMenuSizeX);  // Right side of the items. We add the longest picture (browse_servers)
    m_menuAY += BODGE_FACTOR_1;

    m_pMenuArea = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage(m_pCanvas));
    m_pMenuArea->LoadFromFile(CORE_MTA_FILLER);
    m_pMenuArea->SetPosition(CVector2D(m_menuAX - m_iXOff, m_menuAY - m_iYOff) + BODGE_FACTOR_5, false);
    m_pMenuArea->SetSize(CVector2D(m_menuBX - m_menuAX, m_menuBY - m_menuAY) + BODGE_FACTOR_6, false);
    m_pMenuArea->SetAlpha(0);
    m_pMenuArea->SetZOrderingEnabled(false);
    m_pMenuArea->SetClickHandler(GUI_CALLBACK_MOUSE(&CMainMenu::OnMenuClick, this));
    m_pMenuArea->SetMouseEnterHandler(GUI_CALLBACK(&CMainMenu::OnMenuEnter, this));
    m_pMenuArea->SetMouseLeaveHandler(GUI_CALLBACK(&CMainMenu::OnMenuExit, this));

    m_pLatestNews = reinterpret_cast<CGUIStaticImage*>(pManager->CreateStaticImage());
    m_pLatestNews->SetParent(m_pCanvas);
    m_pLatestNews->SetVisible(false);

    for (uint i = 0; i < CORE_MTA_NEWS_ITEMS; i++)
    {
        m_pNewsItemLabels[i] = reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pCanvas, " "));
        m_pNewsItemLabels[i]->SetVisible(false);
        m_pNewsItemShadowLabels[i] = reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pCanvas, " "));
        m_pNewsItemShadowLabels[i]->SetVisible(false);
        m_pNewsItemDateLabels[i] = reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pCanvas, " "));
        m_pNewsItemDateLabels[i]->SetVisible(false);
        m_pNewsItemNEWLabels[i] = reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pCanvas, " "));
        m_pNewsItemNEWLabels[i]->SetVisible(false);
    }

    m_pLogo->MoveToBack();

    // Submenus
    m_ServerBrowser.SetVisible(false);
    m_ServerInfo.Hide();
    m_Settings.SetVisible(false);
    m_Credits.SetVisible(false);
    m_pNewsBrowser->SetVisible(false);
    m_pLanguageSelector = nullptr;

    // We're not ingame
    SetIsIngame(false);

    // Discord
    if (g_pCore->GetCVars()->GetValue("allow_discord_rpc", false))
    {
        auto discord = g_pCore->GetDiscord();
        if (!discord->IsDiscordRPCEnabled())
            discord->SetDiscordRPCEnabled(true);

        discord->SetPresenceState(_("Main menu"), false);
        discord->SetPresenceStartTimestamp(0);
    }
    // Store the pointer to the graphics subsystem
    m_pGraphics = CGraphics::GetSingletonPtr();

    // Load the server lists
    CXMLNode* pConfig = CCore::GetSingletonPtr()->GetConfig();
    m_ServerBrowser.LoadServerList(pConfig->FindSubNode(CONFIG_NODE_SERVER_FAV), CONFIG_FAVOURITE_LIST_TAG, m_ServerBrowser.GetFavouritesList());
    m_ServerBrowser.LoadServerList(pConfig->FindSubNode(CONFIG_NODE_SERVER_REC), CONFIG_RECENT_LIST_TAG, m_ServerBrowser.GetRecentList());
    m_ServerBrowser.LoadServerList(pConfig->FindSubNode(CONFIG_NODE_SERVER_HISTORY), CONFIG_HISTORY_LIST_TAG, m_ServerBrowser.GetHistoryList());

    // Remove unused node
    if (CXMLNode* pOldNode = pConfig->FindSubNode(CONFIG_NODE_SERVER_INT))
        pConfig->DeleteSubNode(pOldNode);

#ifdef CI_BUILD
    // Add feature branch alert
    m_pFeatureBranchAlertTexture.reset(reinterpret_cast<CGUITexture*>(m_pManager->CreateTexture()));
    std::int32_t buffer = 0xFFFF0000;
    m_pFeatureBranchAlertTexture->LoadFromMemory(&buffer, 1, 1);  // HACK: Load red dot

    m_pFeatureBranchAlertImage.reset(reinterpret_cast<CGUIStaticImage*>(m_pManager->CreateStaticImage(m_pBackground)));
    m_pFeatureBranchAlertImage->LoadFromTexture(m_pFeatureBranchAlertTexture.get());
    m_pFeatureBranchAlertImage->SetPosition({0.0f, 0.0f}, false);
    m_pFeatureBranchAlertImage->SetSize({ScreenSize.fX, 35.0f});

    m_pFeatureBranchAlertLabel.reset(reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pFeatureBranchAlertImage.get())));
    m_pFeatureBranchAlertLabel->SetText(
        _("You are using a feature-branch build! This is a test build only which cannot be used to connect to public servers!"));
    m_pFeatureBranchAlertLabel->SetPosition({0.0f, 0.0f}, false);
    m_pFeatureBranchAlertLabel->SetSize({ScreenSize.fX, 35.0f});
    m_pFeatureBranchAlertLabel->SetFont("clear-normal");
    m_pFeatureBranchAlertLabel->SetHorizontalAlign(CGUI_ALIGN_HORIZONTALCENTER);
    m_pFeatureBranchAlertLabel->SetVerticalAlign(CGUI_ALIGN_VERTICALCENTER);
#endif

#if _WIN32_WINNT <= _WIN32_WINNT_WINXP
    // Add annonying alert
    m_pAlertTexture.reset(reinterpret_cast<CGUITexture*>(m_pManager->CreateTexture()));
    std::int32_t buffer = 0xFFFF0000;
    m_pAlertTexture->LoadFromMemory(&buffer, 1, 1);  // HACK: Load red dot

    m_pAlertImage.reset(reinterpret_cast<CGUIStaticImage*>(m_pManager->CreateStaticImage(m_pBackground)));
    m_pAlertImage->LoadFromTexture(m_pAlertTexture.get());
    m_pAlertImage->SetPosition({0.0f, 0.0f}, false);
    m_pAlertImage->SetSize({ScreenSize.fX, 20.0f});

    #define XP_VISTA_WARNING _("MTA will not receive updates on XP/Vista after July 2019.\n\nUpgrade Windows to play on the latest servers.")
    m_pAlertLabel.reset(reinterpret_cast<CGUILabel*>(m_pManager->CreateLabel(m_pAlertImage.get(), XP_VISTA_WARNING)));
    m_pAlertLabel->SetPosition({0.0f, 2.0f}, false);
    m_pAlertLabel->SetSize({ScreenSize.fX, 20.0f});
    m_pAlertLabel->SetHorizontalAlign(CGUI_ALIGN_HORIZONTALCENTER);
#endif
}

CMainMenu::~CMainMenu()
{
    auto destroyElement = [this](auto*& element)
    {
        if (!element)
            return;
        m_pManager->DestroyElementRecursive(element);
        element = nullptr;
    };

    for (uint i = 0; i < CORE_MTA_NEWS_ITEMS; ++i)
    {
        destroyElement(m_pNewsItemLabels[i]);
        destroyElement(m_pNewsItemShadowLabels[i]);
        destroyElement(m_pNewsItemDateLabels[i]);
        destroyElement(m_pNewsItemNEWLabels[i]);
    }

    for (sMenuItem* pItem : m_menuItems)
    {
        if (!pItem || pItem == m_pDisconnect)
            continue;

        if (pItem->image)
        {
            m_pManager->DestroyElementRecursive(pItem->image);
            pItem->image = nullptr;
        }

        delete pItem;
    }
    m_menuItems.clear();
    m_unhoveredItems.clear();
    m_pHoveredItem = nullptr;

    if (m_pDisconnect)
    {
        if (m_pDisconnect->image)
        {
            m_pManager->DestroyElementRecursive(m_pDisconnect->image);
            m_pDisconnect->image = nullptr;
        }

        delete m_pDisconnect;
        m_pDisconnect = nullptr;
    }

    delete m_pLanguageSelector;
    m_pLanguageSelector = nullptr;

    delete m_pNewsBrowser;
    m_pNewsBrowser = nullptr;

    destroyElement(m_pMenuArea);
    destroyElement(m_pLogo);
    destroyElement(m_pLatestNews);
    destroyElement(m_pVersion);

    destroyElement(m_pCanvas);
    destroyElement(m_pBackground);
    destroyElement(m_pFiller);
    destroyElement(m_pFiller2);
}

void CMainMenu::SetMenuVerticalPosition(int iPosY)
{
    if (m_pHoveredItem)
    {
        m_unhoveredItems.insert(m_pHoveredItem);
        m_pHoveredItem = NULL;
    }

    float fFirstItemSize = m_menuItems.front()->image->GetSize(false).fY;
    int   iMoveY = iPosY - m_menuItems.front()->image->GetPosition(false).fY - fFirstItemSize * 0.5f;

    std::deque<sMenuItem*>::iterator it = m_menuItems.begin();
    for (it; it != m_menuItems.end(); it++)
    {
        CVector2D vOrigPos = (*it)->image->GetPosition(false);
        (*it)->drawPositionY = (*it)->drawPositionY + iMoveY;
        (*it)->image->SetPosition(CVector2D(vOrigPos.fX, vOrigPos.fY + iMoveY), false);
    }

    m_menuAY = m_menuAY + iMoveY;
    m_menuBY = m_menuBY + iMoveY;
    m_pMenuArea->SetPosition(CVector2D(m_menuAX - m_iXOff, m_menuAY - m_iYOff) + BODGE_FACTOR_5, false);
    m_pMenuArea->SetSize(CVector2D(m_menuBX - m_menuAX, m_menuBY - m_menuAY) + BODGE_FACTOR_6, false);
}

void CMainMenu::SetMenuUnhovered()
{
    m_pHoveredItem = NULL;
    std::deque<sMenuItem*>::iterator it = m_menuItems.begin();
    for (it; it != m_menuItems.end(); it++)
    {
        SetItemHoverProgress((*it), 0, false);
    }
}

void CMainMenu::Update()
{
    if (g_pCore->GetDiagnosticDebug() == EDiagnosticDebug::JOYSTICK_0000)
        m_bHideGame = false;

    if (m_bFrameDelay)
    {
        m_bFrameDelay = false;
        return;
    }

    // Get the game interface and the system state
    CGame*      pGame = CCore::GetSingleton().GetGame();
    SystemState systemState = pGame->GetSystemState();

    m_Credits.Update();
    m_Settings.Update();

    unsigned long ulCurrentTick = GetTickCount32();
    unsigned long ulTimePassed = ulCurrentTick - ulPreviousTick;

    if (m_ucFade == FADE_IN || m_ucFade == FADE_OUT || m_ucFade == FADE_VISIBLE)
    {
        float fMaxAlpha = m_bIsIngame ? CORE_MTA_BG_INGAME_ALPHA : CORE_MTA_BG_MAX_ALPHA;
        float fAlpha = Clamp(0.f, m_fFader, fMaxAlpha);
        unsigned char alpha = (unsigned char)(fAlpha * 255.0f);
        m_pGraphics->DrawRectangle(0, 0, m_ScreenSize.fX, m_ScreenSize.fY, (alpha << 24) | 0x000000);
    }


    if (m_bIsFullyVisible)
    {
        // Grab our cursor position
        tagPOINT cursor;
        GetCursorPos(&cursor);

        HWND hookedWindow = CCore::GetSingleton().GetHookedWindow();

        tagPOINT windowPos = {0};
        ClientToScreen(hookedWindow, &windowPos);

        CVector2D vecResolution = CCore::GetSingleton().GetGUI()->GetResolution();
        cursor.x -= windowPos.x;
        cursor.y -= windowPos.y;
        if (cursor.x < 0)
            cursor.x = 0;
        else if (cursor.x > (long)vecResolution.fX)
            cursor.x = (long)vecResolution.fX;
        if (cursor.y < 0)
            cursor.y = 0;
        else if (cursor.y > (long)vecResolution.fY)
            cursor.y = (long)vecResolution.fY;

        // If we're within our highlight bounding box
        if (m_bMouseOverMenu && (cursor.x > m_menuAX) && (cursor.y > m_menuAY + BODGE_FACTOR_3) && (cursor.x < m_menuBX) &&
            (cursor.y < m_menuBY + BODGE_FACTOR_4))
        {
            float fHoveredIndex = ((cursor.y - m_menuAY) / (float)(m_menuBY - m_menuAY) * m_menuItems.size());
            fHoveredIndex = Clamp<float>(0, fHoveredIndex, m_menuItems.size() - 1);
            sMenuItem* pItem = m_menuItems[(int)floor(fHoveredIndex)];
            int        iSizeX = (pItem->nativeSizeX / NATIVE_RES_X) * m_iMenuSizeX;

            if (cursor.x < (iSizeX + m_menuAX))
            {
                if ((m_pHoveredItem) && (m_pHoveredItem != pItem))
                {
                    m_unhoveredItems.insert(m_pHoveredItem);
                    m_pHoveredItem = pItem;
                }
                else
                {
                    m_pHoveredItem = NULL;
                }
                m_pHoveredItem = pItem;
            }
            else if (m_pHoveredItem)
            {
                m_unhoveredItems.insert(m_pHoveredItem);
                m_pHoveredItem = NULL;
            }

            if (m_pHoveredItem)
            {
                float progress = m_pHoveredItem->animProgress + ((float)ulTimePassed / CORE_MTA_ANIMATION_TIME_IN);
                MapRemove(m_unhoveredItems, m_pHoveredItem);
                SetItemHoverProgress(m_pHoveredItem, progress, true);
            }
        }
        else if (m_pHoveredItem)
        {
            m_unhoveredItems.insert(m_pHoveredItem);
            m_pHoveredItem = NULL;
        }

        // Let's unhover our recently un-moused over items
        std::set<sMenuItem*>::iterator it = m_unhoveredItems.begin();
        while (it != m_unhoveredItems.end())
        {
            // Let's work out what the target progress should be by working out the time passed
            // Min of 0.5 progress fixes occasional graphical glitchekal
            float newProgress =
                (*it)->animProgress - std::min(0.5f, ((float)ulTimePassed / CORE_MTA_ANIMATION_TIME_OUT) * (CORE_MTA_HOVER_ALPHA - CORE_MTA_NORMAL_ALPHA));
            if (SetItemHoverProgress((*it), newProgress, false))
            {
                std::set<sMenuItem*>::iterator itToErase = it++;
                m_unhoveredItems.erase(itToErase);
            }
            else
                it++;
        }
    }

    if (m_ucFade == FADE_IN)
    {
        m_fFader += CORE_MTA_FADER;
        float fFadeTarget = m_bIsIngame ? CORE_MTA_BG_INGAME_ALPHA : CORE_MTA_BG_MAX_ALPHA;
        m_pCanvas->SetAlpha(Clamp<float>(0.f, m_fFader, CORE_MTA_BG_MAX_ALPHA));

        if (m_fFader > 0.0f)
        {
            m_bIsVisible = true;
            if (!m_bCursorAlphaReset)
            {
                CGUI* pGUI = g_pCore->GetGUI();
                if (pGUI)
                {
                    pGUI->SetCursorAlpha(1.0f);
                    m_bCursorAlphaReset = true;
                }
            }
        }

        if (m_fFader >= fFadeTarget)
        {
            m_ucFade = FADE_VISIBLE;
            m_bIsVisible = true;
            m_bIsFullyVisible = true;
        }
    }
    else if (m_ucFade == FADE_OUT)
    {
        m_fFader -= CORE_MTA_FADER;
        m_pCanvas->SetAlpha(Clamp(0.f, m_fFader, CORE_MTA_BG_MAX_ALPHA));

        if (m_fFader < 1.0f)
        {
            m_bIsVisible = false;
            m_bCursorAlphaReset = false;
        }

        if (m_fFader <= 0)
        {
            m_bIsFullyVisible = false;
            m_ucFade = FADE_INVISIBLE;
            m_bIsVisible = false;
            m_pCanvas->SetVisible(false);
        }
    }

    // Force the mainmenu on if we're at GTA's mainmenu or not ingame
    if ((systemState == SystemState::GS_FRONTEND || systemState == SystemState::GS_PLAYING_GAME) && !m_bIsIngame)
    {
        if (!m_bStarted)
        {
            m_bStarted = true;
            GetVersionUpdater()->EnableChecking(true);
        }
    }

    // If we're visible
    if (m_bIsVisible && systemState != SystemState::GS_INIT_PLAYING_GAME)
    {
        // If we're at the game's mainmenu, or ingame when m_bIsIngame is true show the background
        if (systemState == SystemState::GS_FRONTEND || systemState == SystemState::GS_PLAYING_GAME && !m_bIsIngame)
        {
            if (m_ucFade == FADE_INVISIBLE)
                Show(false);
        }
        else
        {
            if (m_ucFade == FADE_INVISIBLE)
                Show(true);
        }
    }
    else
    {
        if (m_ucFade == FADE_VISIBLE)
            Hide();
    }

    ulPreviousTick = GetTickCount32();

    // Call subdialog pulses
    m_ServerBrowser.Update();
    m_ServerInfo.DoPulse();
    if (m_pLanguageSelector)
        m_pLanguageSelector->DoPulse();
}

void CMainMenu::Show(bool bOverlay)
{
    SetVisible(true, bOverlay);
}

void CMainMenu::Hide()
{
    SetVisible(false);
}

// When escape key pressed and not connected, hide these windows
void CMainMenu::OnEscapePressedOffLine()
{
    m_ServerBrowser.SetVisible(false);
    m_Credits.SetVisible(false);
    m_pNewsBrowser->SetVisible(false);
}

void CMainMenu::SetVisible(bool bVisible, bool bOverlay, bool bFrameDelay)
{
    CMultiplayer* pMultiplayer = CCore::GetSingleton().GetMultiplayer();
    CQuestionBox* pQuestionBox = CCore::GetSingleton().GetLocalGUI()->GetMainMenu()->GetQuestionWindow();
    pMultiplayer->DisablePadHandler(bVisible);

    if ((m_ucFade == FADE_VISIBLE || m_ucFade == FADE_IN) && bVisible == false)
    {
        m_ucFade = FADE_OUT;
    }
    else if ((m_ucFade == FADE_INVISIBLE || m_ucFade == FADE_OUT) && bVisible == true)
    {
        m_ucFade = FADE_IN;
    }

    // If we're hiding, hide any subwindows we might've had (prevent escaping hiding mousecursor issue)
    if (!bVisible)
    {
        m_bFrameDelay = bFrameDelay;
        SetMenuUnhovered();
        m_ServerBrowser.SetVisible(false);
        m_Settings.SetVisible(false);
        m_Credits.SetVisible(false);
        m_pNewsBrowser->SetVisible(false);

        if (GetIsIngame() && pQuestionBox->IsVisible())
        {
            pQuestionBox->Reset();
            pQuestionBox->Hide();
        }
        //        m_bIsInSubWindow = false;
    }
    else
    {
        m_bFrameDelay = bFrameDelay;
        SetMenuUnhovered();
        m_pCanvas->SetVisible(true);
    }

    m_bHideGame = !bOverlay;
}

bool CMainMenu::IsVisible()
{
    return m_bIsVisible;
}

void CMainMenu::SetIsIngame(bool bIsIngame)
{
    if (m_bIsIngame != bIsIngame)
    {
        m_bIsIngame = bIsIngame;
        m_Settings.SetIsModLoaded(bIsIngame);
        CCore::GetSingleton().RecalculateFrameRateLimit(-1, false);
    }
}

bool CMainMenu::GetIsIngame()
{
    return m_bIsIngame;
}

bool CMainMenu::OnMenuEnter(CGUIElement* pElement)
{
    m_bMouseOverMenu = true;
    return true;
}

bool CMainMenu::OnMenuExit(CGUIElement* pElement)
{
    m_bMouseOverMenu = false;
    return true;
}

bool CMainMenu::OnMenuClick(CGUIMouseEventArgs Args)
{
    CGUIElement* pElement = Args.pWindow;

    // Only handle all our clicks to the menu from here
    if (!m_pHoveredItem)
        return true;

    if (Args.button != LeftButton)
        return true;

    // For detecting startup problems
    WatchDogUserDidInteractWithMenu();

    if (m_pHoveredItem->menuType == MENU_ITEM_SETTINGS)
        OnSettingsButtonClick(pElement);
    else if (m_pHoveredItem->menuType == MENU_ITEM_QUIT)
        OnQuitButtonClick(pElement);

    return true;
}

bool CMainMenu::OnQuickConnectButtonClick(CGUIElement* pElement, bool left)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    if (left)
    {
        if (!g_pCore->IsNetworkReady())
        {
            ShowNetworkNotReadyWindow();
            return true;
        }

        g_pCore->GetCommands()->Execute("reconnect", "");
    }
    else
    {
        m_ServerBrowser.SetVisible(true);
        m_ServerBrowser.OnQuickConnectButtonClick();
    }

    return true;
}

bool CMainMenu::OnResumeButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    SetVisible(false);
    return true;
}

bool CMainMenu::OnBrowseServersButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    //    if ( !m_bIsInSubWindow )
    {
        m_ServerBrowser.SetVisible(true);
        //        m_bIsInSubWindow = true;
    }

    return true;
}

void CMainMenu::HideServerInfo()
{
    m_ServerInfo.Hide();
}

bool CMainMenu::OnDisconnectButtonClick()
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    // Send "disconnect" command to the command handler
    CCommands::GetSingleton().Execute("disconnect", "");

    return true;
}

bool CMainMenu::OnHostGameButtonClick()
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    // Load deathmatch, but with local play
    CModManager::GetSingleton().RequestLoad("local");

    return true;
}

bool CMainMenu::OnEditorButtonClick()
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    // Load deathmatch, but with local play
    CModManager::GetSingleton().RequestLoad("editor");

    return true;
}

bool CMainMenu::OnSettingsButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    //    if ( !m_bIsInSubWindow )
    {
        m_Settings.SetVisible(true);

        // Jax: not sure why this was commented, need it here if our main-menu is set to AlwaysOnTop
        //        m_bIsInSubWindow = true;
    }

    return true;
}

bool CMainMenu::OnAboutButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    // Determine if we're ingame or if the background is set to static
    // If so, show the old credits dialog
    m_Credits.SetVisible(true);

    return true;
}

bool CMainMenu::OnQuitButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    // Send "exit" command to the command handler
    CCommands::GetSingleton().Execute("exit", "");

    return true;
}

bool CMainMenu::OnNewsButtonClick(CGUIElement* pElement)
{
    // Return if we haven't faded in yet
    if (m_ucFade != FADE_VISIBLE)
        return false;

    int iIndex = 0;
    if (pElement)
    {
        // Calc index from color
        CGUIColor color = dynamic_cast<CGUILabel*>(pElement)->GetTextColor();
        for (uint i = 0; i < NUMELMS(headlineColors); i++)
            if (color.R == headlineColors[i].R)
                iIndex = i;
    }

    m_pNewsBrowser->SetVisible(true);
    m_pNewsBrowser->SwitchToTab(iIndex);

    return true;
}

sMenuItem* CMainMenu::CreateItem(unsigned char menuType, const char* szFilename, CVector2D vecRelPosition)
{
    CGUIStaticImage* pImage = reinterpret_cast<CGUIStaticImage*>(m_pManager->CreateStaticImage());

    if (!pImage->LoadFromFile(PathJoin(g_pCore->GetLocalization()->GetLanguageDirectory(), szFilename)))
    {
        // Load en_US if no localization is available
        auto pLanguage = g_pLocalization->GetLanguage("en_US");
        pImage->LoadFromFile(PathJoin(g_pCore->GetLocalization()->GetLanguageDirectory(pLanguage), szFilename));
    }

    // Make our positions absolute
    int iPosX = vecRelPosition.fX * m_iMenuSizeX;
    int iPosY = vecRelPosition.fY * m_iMenuSizeY;

    // Make our sizes relative to the size of menu, but in absolute coordinates
    CVector2D vecNativeSize;
    pImage->GetNativeSize(vecNativeSize);
    int iSizeX = (vecNativeSize.fX / NATIVE_RES_X) * m_iMenuSizeX;
    int iSizeY = (vecNativeSize.fY / NATIVE_RES_Y) * m_iMenuSizeY;

    // Mark our bounding box's bottom value.
    m_menuBY = (iPosY + (iSizeY * CORE_MTA_HOVER_SCALE) / 2) + m_iYOff;
    m_menuBY += BODGE_FACTOR_2;

    // Reduced their size down to unhovered size.
    iSizeX = iSizeX * CORE_MTA_NORMAL_SCALE;
    iSizeY = iSizeY * CORE_MTA_NORMAL_SCALE;
    // Grab our draw position from which we enlarge from
    iPosY = iPosY - (iSizeY / 2);

    pImage->SetParent(m_pCanvas);
    pImage->SetPosition(CVector2D(iPosX, iPosY), false);
    pImage->SetSize(CVector2D(iSizeX, iSizeY), false);
    pImage->SetProperty("InheritsAlpha", "False");
    pImage->SetAlpha(CORE_MTA_NORMAL_ALPHA);

    sMenuItem* s = new sMenuItem();
    s->menuType = menuType;
    s->drawPositionX = vecRelPosition.fX * m_iMenuSizeX;
    s->drawPositionY = vecRelPosition.fY * m_iMenuSizeY;
    s->nativeSizeX = vecNativeSize.fX;
    s->nativeSizeY = vecNativeSize.fY;
    s->image = pImage;
    return s;
}

bool CMainMenu::SetItemHoverProgress(sMenuItem* pItem, float fProgress, bool bHovering)
{
    fProgress = Clamp<float>(0, fProgress, 1);

    pItem->animProgress = fProgress;

    // Always use OutQuad for both hovering and unhovering, to avoid animation glitches.
    fProgress = -fProgress * (fProgress - 2);

    // Work out the target scale
    float fTargetScale = (CORE_MTA_HOVER_SCALE - CORE_MTA_NORMAL_SCALE) * (fProgress) + CORE_MTA_NORMAL_SCALE;

    // Work out our current progress based upon the alpha value now
    pItem->image->SetAlpha((CORE_MTA_HOVER_ALPHA - CORE_MTA_NORMAL_ALPHA) * (fProgress) + CORE_MTA_NORMAL_ALPHA);

    int iSizeX = (pItem->nativeSizeX / NATIVE_RES_X) * m_iMenuSizeX * fTargetScale;
    int iSizeY = (pItem->nativeSizeY / NATIVE_RES_Y) * m_iMenuSizeY * fTargetScale;

    // Aligned to the left horizontally, aligned to the centre vertically
    int iPosX = pItem->drawPositionX;
    int iPosY = (pItem->drawPositionY) - (iSizeY * 0.5);

    pItem->image->SetPosition(CVector2D(iPosX, iPosY), false);
    pItem->image->SetSize(CVector2D(iSizeX, iSizeY), false);

    // Return whether the hovering has maxed out
    return bHovering ? (pItem->animProgress >= 1.0) : (pItem->animProgress <= 0.0f);
}

void CMainMenu::SetNewsHeadline(int iIndex, const SString& strHeadline, const SString& strDate, bool bIsNew)
{
}

void CMainMenu::ReloadNews()
{
}

/////////////////////////////////////////////////////////////
//
// CMainMenu::AskUserIfHeWantsToDisconnect
//
// Ask user if he wants to disconnect
//
/////////////////////////////////////////////////////////////
void CMainMenu::AskUserIfHeWantsToDisconnect(uchar menuType)
{
    SStringX strMessage(
        _("This will disconnect you from the current server."
          "\n\nAre you sure you want to disconnect?"));
    CQuestionBox* pQuestionBox = CCore::GetSingleton().GetLocalGUI()->GetMainMenu()->GetQuestionWindow();
    pQuestionBox->Reset();
    pQuestionBox->SetTitle(_("DISCONNECT WARNING"));
    pQuestionBox->SetMessage(strMessage);
    pQuestionBox->SetButton(0, _("No"));
    pQuestionBox->SetButton(1, _("Yes"));
    pQuestionBox->SetCallback(StaticWantsToDisconnectCallBack, (void*)menuType);
    pQuestionBox->Show();
}

/////////////////////////////////////////////////////////////
//
// CMainMenu::ShowNetworkNotReadyWindow
//
// Shows a window with information that the network module is not ready.
//
/////////////////////////////////////////////////////////////
void CMainMenu::ShowNetworkNotReadyWindow()
{
    static auto HideQuestionWindow = [](void* window, uint) { reinterpret_cast<CQuestionBox*>(window)->Hide(); };

    CQuestionBox& window = m_QuestionBox;
    window.Reset();
    window.SetTitle(_("INFORMATION"));
    window.SetMessage("\n\nThe network module is not ready.\nPlease wait a moment and try again.");
    window.SetButton(0, _("OK"));
    window.SetCallback(HideQuestionWindow, &window);
    window.Show();
}

/////////////////////////////////////////////////////////////
//
// CMainMenu::StaticWantsToDisconnectCallBack
//
// Callback from disconnect question
//
/////////////////////////////////////////////////////////////
void CMainMenu::StaticWantsToDisconnectCallBack(void* pData, uint uiButton)
{
    CLocalGUI::GetSingleton().GetMainMenu()->WantsToDisconnectCallBack(pData, uiButton);
}

/////////////////////////////////////////////////////////////
//
// CMainMenu::WantsToDisconnectCallBack
//
// Callback from disconnect question.
// If user says it's ok, continue with menu item selection
//
/////////////////////////////////////////////////////////////
void CMainMenu::WantsToDisconnectCallBack(void* pData, uint uiButton)
{
    CCore::GetSingleton().GetLocalGUI()->GetMainMenu()->GetQuestionWindow()->Reset();

    if (uiButton == 1)
    {
        int menuType = (int)pData;
        switch (menuType)
        {
            case MENU_ITEM_HOST_GAME:
                OnHostGameButtonClick();
                break;
            case MENU_ITEM_MAP_EDITOR:
                OnEditorButtonClick();
                break;
            case MENU_ITEM_DISCONNECT:
                OnDisconnectButtonClick();
                break;
            default:
                break;
        }
    }
}
