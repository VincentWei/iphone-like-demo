
/* 
** $Id: helloworld.c 481 2008-02-15 06:31:50Z wangjian $
**
** Listing 2.1
**
** helloworld.c: Sample program for MiniGUI Programming Guide
**      The first MiniGUI application.
**
** Copyright (C) 2004 ~ 2007 Feynman Software.
**
** License: GPL
*/

#include <stdio.h>
#include <string.h>

#include <minigui/common.h>
#undef _USE_MINIGUIENTRY
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "sysmain.h"
#include <eblistview.h>
#include <ebcontrol.h>
#include "common_animates/common_animates.h"
#include "../agg/agg_flip.h"
#include "../sharedbuff.h"

#define IDC_SYSMAIN_EBLIST 100

#define MSG_SHAREBUFFER_READY    5000

#if 0
extern void Agg_Flip(HDC hdc, const RECT *rt);
extern BOOL InitFlipAnimation();
extern BOOL LoadRearBitmapFromDC(HWND hwnd);
extern BOOL LoadFrontBitmapFromDC(HWND hwnd);
#endif

HWND g_hBusyWnd;
static BOOL g_bEraseBKGND;
static BITMAP g_BackBitmap;
static BITMAP arrow_bmp;
static BITMAP browser_bmp;
static BITMAP datetime_bmp;
static BITMAP lookfeel_bmp;
static BITMAP network_bmp;
static BITMAP screen_bmp;
static BITMAP wifi_bmp;
//static BITMAP g_Caption;
//static BITMAP g_ListItemBKG;
static BITMAP g_ListItem[6];
static HWND g_hEBListView;
static HWND g_hSubMenuWnd[6];
static BITMAP g_BitmapSub;
static BITMAP g_MainBitmap;

static BOOL g_bFirstEnterWiFi; 
static BOOL g_bFirstEnterEth; 
static BOOL g_bMainToSub;

static EBLVEXTDATA g_ebListData =
{
    47,                   //nItemHeight            
    0,                    //nItemGap               
    0xb4a7,               //nSelectBKColor    
    0x0000,               //nSelectTextColor    
    0x3bb4,               //nItemBKColor       
    0xffff,               //nItemTextColor     
    0xb4a7,               //nMouseOverBkColor   
    0x0,                  //nMouseOverTextColor
    (DWORD)0,             //&g_ListItemBKG,//nItemBKImage 
    (DWORD)0             //&g_ListItemBKG,//(DWORD)&listBkbmp     //nBKImage 
};
static PBITMAP g_pBitmap[3] = {&g_ListItem[0], &g_ListItem[1], &g_ListItem[2]}; 


int CloseSYSIMEMethod (BOOL open)
{
    REQUEST req;
    req.id       = 0x000f;
    req.data     = &open;
    req.len_data = sizeof (BOOL);
    return ClientRequest(&req, NULL, 0);
}

static void DrawBKGDefault (HDC hdc, const RECT* pRect, void* pParam)
{
    //do nothing
}

static void DrawAnimateDefault (HDC hdc, ANIMATE* pAnimate)
{
    if (GetAnimateW (pAnimate) != 0 && GetAnimateH (pAnimate) != 0) 
    {
        FillBoxWithBitmap (hdc, GetAnimateX (pAnimate), GetAnimateY (pAnimate),
                           GetAnimateW (pAnimate), GetAnimateH (pAnimate), 
                           (PBITMAP)pAnimate->img); 
    }
}

static void DrawEndFrameDefault (ANIMATE_SENCE* pAnimateSence)
{
    if (pAnimateSence != NULL)
    {
        BitBlt (pAnimateSence->hdc, 0, 0, RECTW (pAnimateSence->rtArea),
                RECTH (pAnimateSence->rtArea), HDC_SCREEN, 0, 0, 0);
    }
}


void PushPullBitmap (HDC hdc, const RECT *rt, PBITMAP bmpPush, PBITMAP bmpPull, int frame_num, BOOL left_to_right)
{
	int w,h;
	w = RECTWP(rt);
	h = RECTHP(rt);
	
        PUSH_PULL_OBJ objs[2] ={
		{bmpPush,left_to_right?-w:w, 0,0,0},
		{bmpPull,0, 0, left_to_right?w:-w, 0}
	};
        
        ANIMATE_OPS stAnimateOps = 
        {
            DrawAnimateDefault,
            DrawBKGDefault,
            NULL,
            NULL,
            DrawEndFrameDefault
        };

	RunPushPullAnimate(hdc, rt, objs, 2, &stAnimateOps, frame_num, NULL);
}


void GUIAPI UpdateAll (HWND hWnd, BOOL fErase)
{
    MSG Msg;
    RECT rect;

    if (fErase)
        SendAsyncMessage (hWnd, MSG_CHANGESIZE, 0, 0);

    SendAsyncMessage (hWnd, MSG_NCPAINT, 0, 0);
    if (fErase)
        InvalidateRect (hWnd, NULL, TRUE);
    else
        InvalidateRect (hWnd, NULL, FALSE);

    GetClientRect(hWnd, &rect);
    SendMessage (hWnd, MSG_PAINT, 0, (LPARAM)&rect);

    while (PeekMessageEx (&Msg, hWnd, MSG_PAINT, MSG_PAINT+1, 
                FALSE, PM_REMOVE)){
        TranslateMessage (&Msg);
        DispatchMessage (&Msg);
    }
}


int DoubleBufferProc (HWND hWnd, HDC private_dc, HDC real_dc, const RECT*update_rc, const RECT* real_rc)
{
    return 0;
}



typedef struct ZorderOpInfo
{
    int     id_op;

    int     idx_znode;
    DWORD   flags;
    HWND    hwnd;
    HWND    main_win;
    RECT    rc; 
    RECT    rcA;
    char    caption[41]; 
} ZORDEROPINFO;


int ClientSetActiveWindow (HWND hWnd)
{
    int ret;
    REQUEST req;
    ZORDEROPINFO info;
    void* pWnd = (void*) hWnd;
    int* pValue;
    
    info.id_op = 7;
    pWnd = pWnd + 196;
    pValue = (int*)pWnd;
    info.idx_znode = *pValue;

    req.id = 0x000D;
    req.data = &info;
    req.len_data = sizeof (ZORDEROPINFO);

    if (ClientRequest (&req, &ret, sizeof (int)) < 0)
        return -1;
    return 0;
}

int ClientShow (HWND hWnd)
{
    int ret;
    REQUEST req;
    ZORDEROPINFO info;
    void* pWnd = (void*) hWnd;
    int* pValue;
    
    info.id_op = 4;
    pWnd = pWnd + 196;
    pValue = (int*)pWnd;
    info.idx_znode = *pValue;

    req.id = 0x000D;
    req.data = &info;
    req.len_data = sizeof (ZORDEROPINFO);

    if (ClientRequest (&req, &ret, sizeof (int)) < 0)
        return -1;
    return 0;
}

int Client2TopMost (HWND hWnd)
{
    int ret;
    REQUEST req;
    ZORDEROPINFO info;
    void* pWnd = (void*) hWnd;
    int* pValue;
    
    info.id_op = 3;
    //pWnd = pWnd + 196;
    //pWnd = ((char*)pWnd) + 216;
    pWnd = ((char*)pWnd) + 192;
    pValue = (int*)pWnd;
    printf("hwnd is %d, znode is %d.\n", hWnd, *pValue);
    info.idx_znode = *pValue;

    req.id = 0x000D;
    req.data = &info;
    req.len_data = sizeof (ZORDEROPINFO);

    if (ClientRequest (&req, &ret, sizeof (int)) < 0)
        return -1;
    return 0;
}

void AnimateMoveBackUp (HWND hWnd)
{
    Client2TopMost (hWnd);
    ClientShow (hWnd);
    ClientSetActiveWindow (hWnd);
}

#define BUSYWIN_REQID    (MAX_SYS_REQID + 7)
void ShowRollAnimate (BOOL bShow)
{
        int ret;
        REQUEST req = {BUSYWIN_REQID, &bShow, sizeof (BOOL)};
        ClientRequest (&req, &ret, sizeof (int));
}

static void EBListViewNotifyProc (HWND hwnd, int id, int code, DWORD addData)
{
    PEBLSTVWDATA pListdata = (PEBLSTVWDATA)addData;
    PEBITEMDATA pCell = NULL;
    if ((pListdata) && (code == ELVN_LBUTTONUP)) {
        pCell = pListdata->pItemSelected;
        switch (pCell->nRows)
        {
            case 1:
#ifdef GOOD_WIFI
                if (g_bFirstEnterWiFi)
                {
                    int nNum = 0;
                    HDC hdc = CreateCompatibleDC (HDC_SCREEN);
                    BitBlt (HDC_SCREEN, 0, 0, 240, 320, hdc, 0, 0, 0);
                    ShowRollAnimate (TRUE);
                    if (g_hSubMenuWnd[0] != HWND_INVALID)
                    {
                        SendMessage (g_hSubMenuWnd [0], MSG_SYS_CLOSE, 0, 0);
                    }
                    g_hSubMenuWnd[0] = CreateWIFIWindow ();
                    nNum = GetSearchAPNum ();
                    printf ("nNum %d\n", nNum);
                    if (nNum)
                    {
                        g_bFirstEnterWiFi = FALSE; 
                    }
                    else
                    {
                        SendMessage (g_hSubMenuWnd [0], MSG_SYS_CLOSE, 0, 0);
                        printf ("WiFiSearch Error ...\n");
                        g_hSubMenuWnd[0] = CreateWiFiManualWindow ();
                    }
                    ShowRollAnimate (FALSE);
                    BitBlt (hdc, 0, 100, 240, 100, HDC_SCREEN, 0, 100, 0);
                    DeleteCompatibleDC (hdc);
                }
                if (g_hSubMenuWnd[0] == HWND_INVALID)
                    break;
                //CreateLocalNetworkWindow 
                {
                    HDC hdc;
                    hdc = GetSecondaryDC (g_hSubMenuWnd [0]);
                    SetSecondaryDC(g_hSubMenuWnd [0], hdc, ON_UPDSECDC_DONOTHING);
                    ShowWindow (g_hSubMenuWnd[0], SW_SHOWNORMAL);
                    UpdateAll (g_hSubMenuWnd [0], TRUE);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_BitmapSub);
                    SetSecondaryDC(g_hSubMenuWnd [0], hdc, NULL);
                    hdc = GetSecondaryDC (g_hSysMain);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_MainBitmap);
                }
                PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_BitmapSub, &g_MainBitmap, 5, FALSE);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[0], 0);
                AnimateMoveBackUp (g_hSubMenuWnd[0]);
                UnloadBitmap (&g_BitmapSub);
                g_bEraseBKGND = TRUE;
#endif
                break;
            case 2:
                if (g_bFirstEnterEth)
                {
                    HDC hdc = CreateCompatibleDC (HDC_SCREEN);
                    BitBlt (HDC_SCREEN, 0, 0, 240, 320, hdc, 0, 0, 0);
                    ShowRollAnimate (TRUE);
                    g_hSubMenuWnd[1] = CreateNetworkWindow ();
                    ShowRollAnimate (FALSE);
                    BitBlt (hdc, 0, 100, 240, 100, HDC_SCREEN, 0, 100, 0);
                    DeleteCompatibleDC (hdc);
                }
                if (g_hSubMenuWnd[1] == HWND_INVALID)
                    break;
#if 1 
                {
                    HDC hdc;
                    hdc = GetSecondaryDC (g_hSubMenuWnd [1]);
                    SetSecondaryDC(g_hSubMenuWnd [1], hdc, ON_UPDSECDC_DONOTHING);
                    ShowWindow (g_hSubMenuWnd[1], SW_SHOWNORMAL);
                    UpdateAll (g_hSubMenuWnd [1], TRUE);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_BitmapSub);
                    SetSecondaryDC(g_hSubMenuWnd [1], hdc, NULL);
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_MainBitmap);
                }

                PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_BitmapSub, &g_MainBitmap, 5, FALSE);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[1], 0);
                AnimateMoveBackUp (g_hSubMenuWnd[1]);
                UnloadBitmap (&g_BitmapSub);
                g_bEraseBKGND = TRUE;
#endif
                break;
            case 3:
                break;
            case 4:
                if (g_hSubMenuWnd[3] == HWND_INVALID)
                    break;
                {
                    HDC hdc;
                    hdc = GetSecondaryDC (g_hSubMenuWnd [3]);
                    SetSecondaryDC(g_hSubMenuWnd [3], hdc, ON_UPDSECDC_DONOTHING);
                    ShowWindow (g_hSubMenuWnd[3], SW_SHOWNORMAL);
                    UpdateAll (g_hSubMenuWnd [3], TRUE);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_BitmapSub);
                    SetSecondaryDC(g_hSubMenuWnd [3], hdc, NULL);
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_MainBitmap);
                }
                PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_BitmapSub, &g_MainBitmap, 5, FALSE);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[3], 0);
                AnimateMoveBackUp (g_hSubMenuWnd[3]);
                UnloadBitmap (&g_BitmapSub);
                break;
            case 5:
                if (g_hSubMenuWnd[4] == HWND_INVALID)
                    break;

                {
                    HDC hdc;
                    hdc = GetSecondaryDC (g_hSubMenuWnd [4]);
                    SetSecondaryDC(g_hSubMenuWnd [4], hdc, ON_UPDSECDC_DONOTHING);
                    ShowWindow (g_hSubMenuWnd[4], SW_SHOWNORMAL);
                    UpdateAll (g_hSubMenuWnd [4], TRUE);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_BitmapSub);
                    SetSecondaryDC(g_hSubMenuWnd [4], hdc, NULL);
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_MainBitmap);
                }
                PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_BitmapSub, &g_MainBitmap, 5, FALSE);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[4], 0);
                AnimateMoveBackUp (g_hSubMenuWnd[4]);
                UnloadBitmap (&g_BitmapSub);
                break;
            case 6:
                if (g_hSubMenuWnd[5] == HWND_INVALID)
                    break;
                {
                    HDC hdc;
                    hdc = GetSecondaryDC (g_hSubMenuWnd [5]);
                    SetSecondaryDC(g_hSubMenuWnd [5], hdc, ON_UPDSECDC_DONOTHING);
                    ShowWindow (g_hSubMenuWnd[5], SW_SHOWNORMAL);
                    UpdateAll (g_hSubMenuWnd [5], TRUE);
                    GetBitmapFromDC (hdc, 0, 0, 240, 320, &g_BitmapSub);
                    SetSecondaryDC(g_hSubMenuWnd [5], hdc, NULL);
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_MainBitmap);
                }

                PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_BitmapSub, &g_MainBitmap, 5, FALSE);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[5], 0);
                AnimateMoveBackUp (g_hSubMenuWnd[5]);
                UnloadBitmap (&g_BitmapSub);
                break;

        }
    }
}



static void drawitemcallback(HWND hWnd, HDC hdc, void* context)
{
    ITEM_DRAW_CON* drawcontext =(ITEM_DRAW_CON *)context;
    RECT  rect;

    if(!drawcontext)
        return ;

    rect =drawcontext->paint_area;
    SetBkMode(hdc,BM_TRANSPARENT);
    if(1 == drawcontext->row && 1 ==drawcontext->total_rows){
        SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x00, 0x00, 0xFF));
        FillBox (hdc, rect.left, rect.top, (rect.right -rect.left), (rect.bottom -rect.top));
    }
    else if(1 == drawcontext->row){
        if(ROWSTATE_SELECTED ==drawcontext->state)
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[1]);
        else
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[0]);
        FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 85, 39, &wifi_bmp);
        FillBoxWithBitmap (hdc, rect.left +217, rect.top +20, 6, 11, &arrow_bmp);
    }
    else if(drawcontext->row <drawcontext->total_rows){
        if(ROWSTATE_SELECTED ==drawcontext->state)
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[5]);
        else
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[4]);
        if(2 ==drawcontext->row)
            FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 178, 39, &network_bmp);
        else if(3 ==drawcontext->row)
            FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 131, 39, &browser_bmp);
        else if(4 ==drawcontext->row)
            FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 126, 39, &datetime_bmp);
        else if(5 ==drawcontext->row)
            FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 167, 39, &screen_bmp);
        FillBoxWithBitmap (hdc, rect.left +217, rect.top +20, 6, 11, &arrow_bmp);
    }
    else if(drawcontext->row ==drawcontext->total_rows){
        if(ROWSTATE_SELECTED ==drawcontext->state)
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[3]);
        else
            FillBoxWithBitmap (hdc, rect.left, rect.top, 234, 47, &g_ListItem[2]);
        FillBoxWithBitmap (hdc, rect.left +6, rect.top +6, 184, 39, &lookfeel_bmp);
        FillBoxWithBitmap (hdc, rect.left +217, rect.top +20, 6, 11, &arrow_bmp);
    }
    else
        printf("Here has an error!file:%s, func:%s\n",__FILE__,__func__);
}

static int SysMainProc(HWND hWnd, int nMessage, WPARAM wParam, LPARAM lParam)
{
    switch ( nMessage )
    {
        case MSG_CREATE:
            {
                EBLVITEM stListViewItem;
                EBLVCOLOUM stListViewColumn;
                EBLVSUBITEM stListViewSubItem;
                char cServiceName[64];
                int i;

                g_bFirstEnterWiFi = TRUE; 
                g_bFirstEnterEth = TRUE; 
                g_bEraseBKGND = FALSE;
                g_hEBListView = CreateWindowEx (CTRL_EBLISTVIEW, 
                        "",
                        WS_CHILD | WS_VISIBLE | ELVS_TYPE3STATE |
                        ELVS_ITEMUSERCOLOR | ELVS_BKIMAGELEFTTOP |
                        ELVS_BKBITMAP,
                        WS_EX_TRANSPARENT,
                        IDC_SYSMAIN_EBLIST,
                        3, 35, 234, 282,
                        hWnd, (DWORD)&g_ebListData);
                SendMessage (g_hEBListView, ELVM_SET_ITEMDRAWCALLBACK, (WPARAM)drawitemcallback,0);

                g_hSysMain = hWnd;
                stListViewColumn.nCols = 1;
                stListViewColumn.pszHeadText = "dkx test";
                stListViewColumn.width = 30;
                SendMessage (g_hEBListView, ELVM_ADDCOLUMN, 0, (LPARAM)&stListViewColumn);

                stListViewColumn.nCols = 2;
                stListViewColumn.pszHeadText = "dkx test";
                stListViewColumn.width = 150;
                SendMessage (g_hEBListView, ELVM_ADDCOLUMN, 0, (LPARAM)&stListViewColumn);

                stListViewColumn.nCols = 3;
                stListViewColumn.pszHeadText = "dkx test";
                stListViewColumn.width = 30;
                SendMessage (g_hEBListView, ELVM_ADDCOLUMN, 0, (LPARAM)&stListViewColumn);
                sprintf (cServiceName, "Test the EBListView");

                for (i = 1; i < 7; i++)
                {
                    stListViewItem.nItem = i;
                    SendMessage (g_hEBListView, ELVM_ADDITEM, 0, (LPARAM) &stListViewItem);
                    stListViewSubItem.nItem = i;
                    stListViewSubItem.mask = ELV_BITMAP; 
                    stListViewSubItem.subItem = 1;
                    stListViewSubItem.pszText = NULL;//(char *)"111111";//&cServiceName;
                    stListViewSubItem.cchTextMax = 0;//strlen ("111111");//cServiceName);
                    stListViewSubItem.iImage = (DWORD)g_pBitmap;
                    stListViewSubItem.wordtype =NULL;
                    SendMessage (g_hEBListView, ELVM_FILLSUBITEM, 0, (LPARAM) &stListViewSubItem);
#if 1 
                    stListViewSubItem.nItem = i;
                    stListViewSubItem.mask = ELV_TEXT; 
                    stListViewSubItem.subItem = 2;
                    stListViewSubItem.pszText = (char *)"222222";//&cServiceName;
                    stListViewSubItem.cchTextMax = strlen ("222222");//cServiceName);
                    stListViewSubItem.wordtype = NULL;
                    stListViewSubItem.iImage = 0;
                    stListViewSubItem.lparam = 0;
                    SendMessage (g_hEBListView, ELVM_FILLSUBITEM, 0, (LPARAM) &stListViewSubItem);
#endif

                    stListViewSubItem.nItem = i;
                    stListViewSubItem.mask = ELV_BITMAP; 
                    stListViewSubItem.subItem = 3;
                    stListViewSubItem.pszText = NULL;//(char *)"111111";//&cServiceName;
                    stListViewSubItem.cchTextMax = 0;//strlen ("111111");//cServiceName);
                    stListViewSubItem.iImage = (DWORD)g_pBitmap;
                    stListViewSubItem.wordtype =NULL;
                    SendMessage (g_hEBListView, ELVM_FILLSUBITEM, 0, (LPARAM) &stListViewSubItem);
                }
                SetNotificationCallback (g_hEBListView, EBListViewNotifyProc); 
            }
            break;
        case MSG_SHAREBUFFER_READY:
            g_bMainToSub = TRUE;
            return 0;
        case MSG_PAINT:
            break;
        case MSG_CLOSE_APP:
            switch (lParam)
            {
                case 6:
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_BitmapSub);
                    SendMessage (g_hSubMenuWnd[5], MSG_SYS_CLOSE, 0, 0);
                    SetWindowStyleType (EM_TYPE_SKIN);
                    g_hSubMenuWnd[5] = CreateRenderWindow ();
                    g_bEraseBKGND  = TRUE;
                    ShowWindow (g_hSubMenuWnd[5], SW_HIDE);
                    PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_MainBitmap, &g_BitmapSub, 5, TRUE);
                    UnloadBitmap (&g_BitmapSub);
                    UnloadBitmap (&g_MainBitmap);
                    break;
                case 5:
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_BitmapSub);
                    g_bEraseBKGND  = TRUE;
                    ShowWindow (g_hSubMenuWnd[4], SW_HIDE);
                    PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_MainBitmap, &g_BitmapSub, 5, TRUE);
                    UnloadBitmap (&g_BitmapSub);
                    UnloadBitmap (&g_MainBitmap);
                    break;
                case 4:
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_BitmapSub);
                    g_bEraseBKGND  = TRUE;
                    ShowWindow (g_hSubMenuWnd[3], SW_HIDE);
                    PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_MainBitmap, &g_BitmapSub, 5, TRUE);
                    UnloadBitmap (&g_BitmapSub);
                    UnloadBitmap (&g_MainBitmap);
                    break;
                case 2:
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_BitmapSub);
                    g_bEraseBKGND  = TRUE;
                    ShowWindow (g_hSubMenuWnd[1], SW_HIDE);
                    PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_MainBitmap, &g_BitmapSub, 5, TRUE);
                    SendMessage (g_hSubMenuWnd [1], MSG_SYS_CLOSE, 0, 0);
                    g_hSubMenuWnd[1] = HWND_INVALID;
                    UnloadBitmap (&g_BitmapSub);
                    UnloadBitmap (&g_MainBitmap);
                    break;
                case 1:
                    GetBitmapFromDC (HDC_SCREEN, 0, 0, 240, 320, &g_BitmapSub);
                    g_bEraseBKGND  = TRUE;
                    ShowWindow (g_hSubMenuWnd[0], SW_HIDE);
                    PushPullBitmap (g_hDoubleBuffer, &g_rcSysSetting, &g_MainBitmap, &g_BitmapSub, 5, TRUE);
                    UnloadBitmap (&g_BitmapSub);
                    UnloadBitmap (&g_MainBitmap);
                    break;
                default:
                    break;
            }
            ShowWindow (hWnd, SW_SHOWNORMAL);
            break;
        case MSG_RENDER_COLOR:
            {
                HWND hTmp;

                InitFlipAnimation();
                LoadFrontBitmapFromDC(g_hSubMenuWnd [5]);

                hTmp = CreateRenderWindow ();
                //IncludeWindowStyle (hTmp, WS_VISIBLE);
                LoadRearBitmapFromDC(hTmp);

                ShowWindow (g_hSubMenuWnd [5], SW_HIDE);
                Agg_Flip( HDC_SCREEN, 30);

                SendMessage (g_hSubMenuWnd[5], MSG_SYS_CLOSE, 0, 0);
                g_hSubMenuWnd[5] = hTmp;
                //IncludeWindowStyle (g_hSubMenuWnd[5], WS_VISIBLE);

                AnimateMoveBackUp (g_hSubMenuWnd[5]);
                SendMessage (HWND_DESKTOP, MSG_MOVETOTOPMOST, (WPARAM)g_hSubMenuWnd[5], 0);

            }
            return 0;
        case MSG_ERASEBKGND:
            {
                HDC hdc = (HDC)wParam;
                const RECT* clip = (const RECT*) lParam;
                BOOL fGetDC = FALSE;
                RECT rcTemp;

                if (g_bEraseBKGND)
                {
                    g_bEraseBKGND = FALSE;
                    return 0;
                }
                if (g_bMainToSub)
                {
                    g_bMainToSub = FALSE;
                    return 0;
                }
                if (hdc == 0){
                    //hdc = GetClientDC (hWnd);
                    hdc = GetSecondaryClientDC(hWnd);
                    fGetDC = TRUE;
                }
                if (clip){
                    rcTemp = *clip;
                    ScreenToClient (hWnd, &rcTemp.left, &rcTemp.top);
                    ScreenToClient (hWnd, &rcTemp.right, &rcTemp.bottom);
                    IncludeClipRect (hdc, &rcTemp);
                }
                FillBoxWithBitmap (hdc, 0, 0, 240, 320, &g_BackBitmap);
                if (fGetDC) {
                    //ReleaseDC (hdc);
                    ReleaseSecondaryDC(hWnd, hdc);
                }
                return 0;
            }
        case MSG_CLOSE:
            DestroyAllControls ( hWnd );
            DestroyMainWindow ( hWnd );
            return 0;
    }
    return DefaultMainWinProc(hWnd, nMessage, wParam, lParam);
}

int MiniGUIMain (int argc, const char* argv[])
{
   MSG Msg;
   HWND hMainWnd;
   MAINWINCREATE CreateInfo;
#ifdef _MGRM_PROCESSES
    JoinLayer(NAME_DEF_LAYER , "systemmainmenu" , 0 , 0);
#endif

   g_hDoubleBuffer = CreateCompatibleDC (HDC_SCREEN);
   
   g_pLogFont = CreateLogFont ("vbf", "Naskhi12", "ISO8859-6",
                                FONT_WEIGHT_BOLD, FONT_SLANT_ROMAN,
                                FONT_SETWIDTH_NORMAL, FONT_SPACING_CHARCELL,
                                FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                                18, 0);
   RegisterEBListViewControl ();
   RegisterMonCalendarControl();
   RegisterMgdButton();

   CreateInfo.dwStyle = WS_NONE;
   CreateInfo.dwExStyle = WS_EX_AUTOSECONDARYDC;
   CreateInfo.spCaption = "";
   CreateInfo.hMenu = 0;
   CreateInfo.hCursor = GetSystemCursor(0);
   CreateInfo.hIcon = 0;
   CreateInfo.MainWindowProc = SysMainProc;
   CreateInfo.lx = 0;
   CreateInfo.ty = 0;
   CreateInfo.rx = 240;
   CreateInfo.by = 320;
   CreateInfo.iBkColor = PIXEL_black;
   CreateInfo.dwAddData = 0;
   CreateInfo.hHosting = HWND_DESKTOP;
   
   g_rcSysSetting.left = 0;
   g_rcSysSetting.top = 0;
   g_rcSysSetting.right = 240;
   g_rcSysSetting.bottom = 320;
   
   LoadBitmap (HDC_SCREEN, &g_BackBitmap, SYSTEM_RES"MainMenuBK.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[0], SYSTEM_RES"upround_nr.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[1], SYSTEM_RES"upround_ov.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[2], SYSTEM_RES"belowround_nr.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[3], SYSTEM_RES"belowround_ov.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[4], SYSTEM_RES"noround_nr.png");
   LoadBitmap (HDC_SCREEN, &g_ListItem[5], SYSTEM_RES"noround_ov.png");
   LoadBitmap (HDC_SCREEN, &arrow_bmp,SYSTEM_RES"arrow.png");
   LoadBitmap (HDC_SCREEN, &browser_bmp,SYSTEM_RES"browsersetting.png");
   LoadBitmap (HDC_SCREEN, &datetime_bmp,SYSTEM_RES"datetime.png");
   LoadBitmap (HDC_SCREEN, &lookfeel_bmp,SYSTEM_RES"lookfeel.png");
   LoadBitmap (HDC_SCREEN, &network_bmp,SYSTEM_RES"localnetwork.png");
   LoadBitmap (HDC_SCREEN, &screen_bmp,SYSTEM_RES"screensaver.png");
   LoadBitmap (HDC_SCREEN, &wifi_bmp,SYSTEM_RES"WI-FI.png");
  
   memset (g_hSubMenuWnd, 0x0, sizeof(g_hSubMenuWnd));
   memset (g_SubMenuBitmap, 0x0, sizeof(g_SubMenuBitmap));
   //Load animate resource
   LoadBitmap (HDC_SCREEN, &g_SubMenuBitmap[0], ANIMATE_RES"SystemSettingMainMenu.bmp");
   
   //UpdateWindow (g_hSubMenuWnd[1], TRUE);
   //CreateDateWindow 
   g_hSubMenuWnd[3] = CreateDateWindow ();
   //UpdateWindow (g_hSubMenuWnd[3], TRUE);
   //CreateScreenSaver 
   g_hSubMenuWnd[4] = CreateScreenSaver ();
   //UpdateWindow (g_hSubMenuWnd[4], TRUE);
   //CreateRenderWindow 
   g_hSubMenuWnd[5] = CreateRenderWindow ();
   //UpdateWindow (g_hSubMenuWnd[5], TRUE);
   hMainWnd = CreateMainWindow (&CreateInfo);
   if (hMainWnd == HWND_INVALID)
       return -1;

   //g_hBusyWnd = CreateBusyWindow (hMainWnd);
#if 0
   ShowWindow (hMainWnd, SW_SHOWNORMAL);
#else
   UpdateWindow (hMainWnd, FALSE);
   ShowWindowUsingShareBuffer (hMainWnd);
#endif

   while (GetMessage(&Msg, hMainWnd)) {
       TranslateMessage(&Msg);
       DispatchMessage(&Msg);
   }
   if (g_hSubMenuWnd[0] == HWND_INVALID)
       SendMessage (g_hSubMenuWnd[0], MSG_SYS_CLOSE, 0, 0);
   if (g_hSubMenuWnd[1] == HWND_INVALID)
       SendMessage (g_hSubMenuWnd[1], MSG_SYS_CLOSE, 0, 0);
   SendMessage (g_hSubMenuWnd[3], MSG_SYS_CLOSE, 0, 0);
   SendMessage (g_hSubMenuWnd[4], MSG_SYS_CLOSE, 0, 0);
   SendMessage (g_hSubMenuWnd[5], MSG_SYS_CLOSE, 0, 0);
   //SendMessage (g_hBusyWnd, MSG_CLOSE, 0, 0);
   
   UnloadBitmap (&g_BackBitmap);
   UnloadBitmap (&g_ListItem[0]);
   UnloadBitmap (&g_ListItem[1]);
   UnloadBitmap (&g_ListItem[2]);
   UnloadBitmap (&g_ListItem[3]);
   UnloadBitmap (&g_ListItem[4]);
   UnloadBitmap (&g_ListItem[5]);
   UnloadBitmap (&arrow_bmp);
   UnloadBitmap (&browser_bmp);
   UnloadBitmap (&datetime_bmp);
   UnloadBitmap (&lookfeel_bmp);
   UnloadBitmap (&network_bmp);
   UnloadBitmap (&screen_bmp);
   UnloadBitmap (&wifi_bmp);
   UnloadBitmap (&g_SubMenuBitmap[0]);
   DeleteMemDC (g_hDoubleBuffer);
   EBListViewControlCleanup ();
   UnregisterMgdButton();
   MonCalendarControlCleanup (); 
   MainWindowThreadCleanup (hMainWnd);
   return 0;

}
