/*****************************************************************************
 *
 *  PROJECT:     New York Chronicles
 *  FILE:        multiplayer_sa/CMultiplayerSA_GPS.cpp
 *  PURPOSE:     GPS waypoint pathfinding and radar line rendering
 *  BASED ON:    https://github.com/juicermv/GTA-GPS-Redux
 *
 *****************************************************************************/

#include "StdInc.h"

extern CGame* pGameInterface;

#define GPS_MAX_NODES 2048
#define GPS_MAX_WAYPOINTS 8

struct CNodeAddress
{
    short sAreaId;
    short sNodeId;
};

struct GPSVec2D
{
    float fX, fY;
};

struct RwIm2DVertex_GPS
{
    float x, y, z, rhw;
    DWORD emissiveColor;
    float u, v;
};

struct GPSWaypoint
{
    bool    active;
    CVector dest;
    DWORD   color;
    float   lineWidth;
    DWORD   lastCalc;
    float   totalDist;
    CVector nodes[GPS_MAX_NODES];
    int     nodeCount;
};

static GPSWaypoint gWaypoints[GPS_MAX_WAYPOINTS];
static int         gNextID = 1;

BYTE gpPathStreamFlags[64];
int  gpPathNodeStorage[10000];

typedef void(__cdecl* tTransform)(GPSVec2D&, GPSVec2D const&);
typedef int(__cdecl* tRwFunc)(int, void*, ...);

static const tTransform fnWorldToRadar = (tTransform)0x583530;
static const tTransform fnRadarToScreen = (tTransform)0x583480;

static void GPS_RecalcWaypoint(GPSWaypoint& wp)
{
    wp.nodeCount = 0;
    wp.totalDist = 0.0f;

    CPed* pLocalPlayer = pGameInterface->GetPools()->GetPedFromRef((DWORD)1);
    if (!pLocalPlayer)
        return;

    CVector vecStart = *pLocalPlayer->GetPosition();
    CVector vecDest = wp.dest;
    BYTE*   pPathFind = (BYTE*)0x96F050;

    static CNodeAddress resultNodes[GPS_MAX_NODES];
    short               nodesFound = 0;
    float               fDist = 0.0f;
    CNodeAddress        invalidAddr = {-1, -1};
    DWORD               dwInvalid = *(DWORD*)&invalidAddr;
    DWORD               dwFunc = 0x4515D0;

    // clang-format off
    __asm
    {
        push    0
        push    0
        push    dwInvalid
        push    0
        push    0x497423C0
        push    0
        push    0x497423C0
        lea     eax, fDist
        push    eax
        push    GPS_MAX_NODES
        lea     eax, nodesFound
        push    eax
        lea     eax, resultNodes
        push    eax
        push    vecDest.fZ
        push    vecDest.fY
        push    vecDest.fX
        push    dwInvalid
        push    vecStart.fZ
        push    vecStart.fY
        push    vecStart.fX
        push    0
        mov     ecx, pPathFind
        call    dwFunc
    }
    // clang-format on

    if (nodesFound <= 0)
        return;

    int iCount = std::min((int)nodesFound, GPS_MAX_NODES);
    for (int i = 0; i < iCount; i++)
    {
        short areaId = resultNodes[i].sAreaId;
        if ((unsigned short)areaId >= 64)
            continue;
        DWORD pNodes = *(DWORD*)(pPathFind + 0x804 + areaId * 4);
        if (!pNodes)
            continue;
        BYTE* pNode = (BYTE*)(pNodes + resultNodes[i].sNodeId * 28);
        wp.nodes[wp.nodeCount].fX = *(short*)(pNode + 0x8) * 0.125f;
        wp.nodes[wp.nodeCount].fY = *(short*)(pNode + 0xA) * 0.125f;
        wp.nodes[wp.nodeCount].fZ = *(short*)(pNode + 0xC) * 0.125f;
        wp.nodeCount++;
    }

    for (int i = 0; i < wp.nodeCount - 1; i++)
    {
        float dx = wp.nodes[i + 1].fX - wp.nodes[i].fX;
        float dy = wp.nodes[i + 1].fY - wp.nodes[i].fY;
        wp.totalDist += sqrtf(dx * dx + dy * dy);
    }
}

static void GPS_RenderWaypoint(GPSWaypoint& wp, IDirect3DDevice9* pDevice, const RECT& sr, tRwFunc fpSetRS, tRwFunc fpRender)
{
    DWORD dwNow = ::GetTickCount32();
    if (dwNow - wp.lastCalc > 500)
    {
        GPS_RecalcWaypoint(wp);
        wp.lastCalc = dwNow;
    }

    if (wp.nodeCount < 2)
        return;

    static GPSVec2D screenNodes[GPS_MAX_NODES];
    for (int i = 0; i < wp.nodeCount; i++)
    {
        GPSVec2D radar, world = {wp.nodes[i].fX, wp.nodes[i].fY};
        fnWorldToRadar(radar, world);
        // Square-radar clip (matches LimitRadarPointSquare)
        if (fabsf(radar.fX) > 1.0f || fabsf(radar.fY) > 1.0f)
        {
            const float kSqrt2 = 1.4142135624f;
            const float kRad2Deg = 57.29577951f;
            const float kDeg2Rad = 1.0f / kRad2Deg;
            float deg = atan2f(radar.fY, radar.fX) * kRad2Deg;
            if (deg > 45.0f && deg <= 135.0f)
                radar.fX = cosf(deg * kDeg2Rad) * kSqrt2, radar.fY = 1.0f;
            else if (deg > -135.0f && deg <= -45.0f)
                radar.fX = cosf(deg * kDeg2Rad) * kSqrt2, radar.fY = -1.0f;
            else if (deg > 135.0f || deg <= -135.0f)
                radar.fX = -1.0f, radar.fY = sinf(deg * kDeg2Rad) * kSqrt2;
            else
                radar.fX = 1.0f, radar.fY = sinf(deg * kDeg2Rad) * kSqrt2;
        }
        fnRadarToScreen(screenNodes[i], radar);
    }

    fpSetRS(1, nullptr);

    float nearZ = *(float*)0x858CAC + 0.0001f;
    float rhw = *(float*)0x858CB0;
    float w = wp.lineWidth;

    static RwIm2DVertex_GPS verts[GPS_MAX_NODES * 4];
    int n = 0;

    for (int i = 0; i < wp.nodeCount - 1 && n < GPS_MAX_NODES * 4 - 4; i++)
    {
        if (screenNodes[i].fX < sr.left || screenNodes[i].fX > sr.right ||
            screenNodes[i].fY < sr.top  || screenNodes[i].fY > sr.bottom)
            continue;

        float dx = screenNodes[i + 1].fX - screenNodes[i].fX;
        float dy = screenNodes[i + 1].fY - screenNodes[i].fY;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.001f)
            continue;
        float px = -dy / len * w;
        float py =  dx / len * w;

        RwIm2DVertex_GPS v = {0, 0, nearZ, rhw, wp.color, 0, 0};
        v.x = screenNodes[i].fX + px;     v.y = screenNodes[i].fY + py;     verts[n]   = v;
        v.x = screenNodes[i].fX - px;     v.y = screenNodes[i].fY - py;     verts[n+1] = v;
        v.x = screenNodes[i+1].fX + px;   v.y = screenNodes[i+1].fY + py;   verts[n+2] = v;
        v.x = screenNodes[i+1].fX - px;   v.y = screenNodes[i+1].fY - py;   verts[n+3] = v;
        n += 4;
    }

    if (n >= 4)
        fpRender(4, verts, n);
}

void GPS_Render()
{
    bool anyActive = false;
    for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
        if (gWaypoints[i].active) { anyActive = true; break; }
    if (!anyActive)
        return;

    IDirect3DDevice9* pDevice = *(IDirect3DDevice9**)0xC97C28;
    if (!pDevice)
        return;

    GPSVec2D c1, c2;
    GPSVec2D r1 = {-1.0f, -1.0f}, r2 = {1.0f, 1.0f};
    fnRadarToScreen(c1, r1);
    fnRadarToScreen(c2, r2);
    RECT sr = {(LONG)(c1.fX + 2), (LONG)(c2.fY + 2), (LONG)(c2.fX - 2), (LONG)(c1.fY - 2)};
    pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    pDevice->SetScissorRect(&sr);

    DWORD pEngine = *(DWORD*)0xC97B24;
    if (!pEngine)
        return;
    tRwFunc fpSetRS = *(tRwFunc*)(pEngine + 0x20);
    tRwFunc fpRender = *(tRwFunc*)(pEngine + 0x30);

    for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
        if (gWaypoints[i].active)
            GPS_RenderWaypoint(gWaypoints[i], pDevice, sr, fpSetRS, fpRender);

    pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

int GPS_SetWaypoint(float fX, float fY, float fZ, DWORD dwColor, float fWidth)
{
    int slot = -1;
    for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
        if (!gWaypoints[i].active) { slot = i; break; }
    if (slot < 0)
        return 0;

    GPSWaypoint& wp = gWaypoints[slot];
    wp.active = true;
    wp.dest = CVector(fX, fY, fZ);
    wp.color = dwColor;
    wp.lineWidth = fWidth;
    wp.lastCalc = 0;
    wp.nodeCount = 0;
    wp.totalDist = 0.0f;

    int id = gNextID++;
    if (gNextID <= 0) gNextID = 1;
    return (slot << 16) | (id & 0xFFFF);
}

bool GPS_ClearWaypoint(int waypointId)
{
    if (waypointId == 0)
    {
        for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
            gWaypoints[i].active = false;
        return true;
    }
    int slot = (waypointId >> 16) & 0xFF;
    if (slot < 0 || slot >= GPS_MAX_WAYPOINTS)
        return false;
    gWaypoints[slot].active = false;
    return true;
}

void GPS_Clear()
{
    GPS_ClearWaypoint(0);
}

bool GPS_IsActive()
{
    for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
        if (gWaypoints[i].active) return true;
    return false;
}

float GPS_GetDistance()
{
    float total = 0.0f;
    for (int i = 0; i < GPS_MAX_WAYPOINTS; i++)
        if (gWaypoints[i].active) total += gWaypoints[i].totalDist;
    return total;
}

float GPS_GetWaypointDistance(int waypointId)
{
    int slot = (waypointId >> 16) & 0xFF;
    if (slot < 0 || slot >= GPS_MAX_WAYPOINTS || !gWaypoints[slot].active)
        return 0.0f;
    return gWaypoints[slot].totalDist;
}

int GPS_GetPathNodes(CVector* outNodes, int maxNodes)
{
    int total = 0;
    for (int w = 0; w < GPS_MAX_WAYPOINTS && total < maxNodes; w++)
    {
        if (!gWaypoints[w].active)
            continue;
        for (int i = 0; i < gWaypoints[w].nodeCount && total < maxNodes; i++)
            outNodes[total++] = gWaypoints[w].nodes[i];
    }
    return total;
}
