/* installer.c

   Part of the swftools installer (Main program).
   
   Copyright (c) 2004 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "depack.h"

#include "../config.h" //for swftools version

extern char*crndata;

static char*install_path = "c:\\swftools\\";

static HWND wnd_progress = 0;
static HWND wnd_params = 0;

#define USER_SETMESSAGE 0x7fff0001

struct progress_data {
    int width,height;
    int bar_width;
    int bar_height;
    int bar_posx;
    int bar_posy;
    int pos,step,range;
    char*text1;
    char*text2;
    char*text3;
    HWND hwndButton;
    HWND wnd_text3;
};
struct params_data {
    int width,height;
    int ok;
    HWND installButton;
    HWND edit;
};

	
LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    printf("%08x, %d %08x %08x\n", hwnd, message, wParam, lParam);
   
    /* in order for the delegation below to also work for
       WM_CREATE, we need to assign our window pointers *before* the
       CreateWindow returns, because that's when the WM_CREATE event 
       is sent  */
    if(message == WM_CREATE) {
	CREATESTRUCT*cs = ((LPCREATESTRUCT)lParam);
	if(cs->lpCreateParams && !strcmp((char*)cs->lpCreateParams, "params")) {
	    wnd_params = hwnd;
	}
	if(cs->lpCreateParams && !strcmp((char*)cs->lpCreateParams, "progress")) {
	    wnd_progress = hwnd;
	}
    }

    if(hwnd == 0) {
	return DefWindowProc(hwnd, message, wParam, lParam);
    } else if(hwnd == wnd_progress) {
	static struct progress_data data;

	switch(message)
	{
	    case USER_SETMESSAGE:
		data.text3 = (char*)wParam;
		SendMessage(data.wnd_text3, WM_SETTEXT, 0, data.text3);
		return 0;
	    case WM_CREATE: {
		memset(&data, 0, sizeof(data));
		data.text1 = "Installing SWFTools";
		data.text2 = (char*)malloc(strlen(install_path)+250);
		sprintf(data.text2, "to %s", install_path);
		data.pos = 0;
		data.step = 1;

		CREATESTRUCT*cs = ((LPCREATESTRUCT)lParam);
		RECT rc;
		GetClientRect (hwnd, &rc);
		data.width = rc.right - rc.left;
		data.height = rc.bottom - rc.top;
		data.bar_width = cs->cx - 17;
		data.bar_height= 16;
		data.bar_posx = (data.width -data.bar_width)/2;
		data.bar_posy = 56;
		data.range = 50;
		data.hwndButton = CreateWindow (
			PROGRESS_CLASS,
			"Progress",
			WS_CHILD | WS_VISIBLE,
			data.bar_posx,
			data.bar_posy,
			data.bar_width, 
			data.bar_height,
			hwnd,  /* Parent */
			(HMENU)1,
			cs->hInstance,
			NULL
			);

		data.wnd_text3 = CreateWindow (
			WC_EDIT,
			"text3",
			WS_CHILD | WS_VISIBLE | ES_READONLY | ES_CENTER,
			data.bar_posx,
			72,
			(rc.right - rc.left - data.bar_posx*2), 
			20,
			hwnd,  /* Parent */
			(HMENU)1,
			cs->hInstance,
			NULL
			);
		SendMessage(data.hwndButton, PBM_SETRANGE, 0, (LPARAM) MAKELONG(0,data.range));
		SendMessage(data.hwndButton, PBM_SETSTEP, (WPARAM) data.step, 0);
		//ShowWindow(hwndButton, SW_SHOW);
		return 0;
	    }   
	    case PBM_STEPIT: {
		if(data.pos+data.step < data.range) {
		    data.pos += data.step;
		    SendMessage(data.hwndButton, PBM_STEPIT, wParam, lParam);
		}
	    }
	    case WM_PAINT: {
		TEXTMETRIC    tm;
		HDC           hdc;             /* A device context used for drawing */
		PAINTSTRUCT   ps;              /* Also used during window drawing */
		RECT          rc;              /* A rectangle used during drawing */
                
                hdc = GetDC(hwnd);
                SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
                GetTextMetrics(hdc, &tm);
                ReleaseDC(hwnd, hdc);

		hdc = BeginPaint (hwnd, &ps);
		
		rc.top = 8; rc.left= 0; rc.right = data.width; rc.bottom = 24;
		DrawText(hdc, data.text1, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		/*if(data.text3) {
		    rc.top = 112; rc.left= 0; rc.right = data.width; rc.bottom = 128;
		    InvalidateRect(hwnd,&rc,1);
		    DrawText(hdc, data.text3, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}*/

		char buf[256];
		char*text = data.text2;
		if(tm.tmAveCharWidth * strlen(text) > data.width) {
		    int chars = (data.width / tm.tmAveCharWidth)-8;
		    if(chars>240) chars=240;
		    strncpy(buf, text, chars);
		    strcpy(&buf[chars],"...");
		    text = buf;
		}

		rc.top = 32; rc.left= 0; rc.right = data.width; rc.bottom = 48;
		DrawText(hdc, text, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		EndPaint (hwnd, &ps);
		return 0;
	    }
	    case WM_DESTROY:
		wnd_progress = 0;
		return DefWindowProc(hwnd, message, wParam, lParam);
	    default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
    } else if(hwnd == wnd_params) {
	static struct params_data data;
	switch(message)
	{
	    case WM_CREATE: {
		memset(&data, 0, sizeof(data));
		CREATESTRUCT*cs = ((LPCREATESTRUCT)lParam);
		RECT rc;
		GetClientRect (hwnd, &rc);
		data.width = rc.right - rc.left;
		data.height = rc.bottom - rc.top;

		//EDITTEXT IDD_EDIT,68,8,72,12, ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER  | WS_TABSTOP
		HWND text = CreateWindow(
			WC_STATIC,
			"Select Installation Directory:",
			WS_CHILD | WS_VISIBLE,
			32, 
			0,
			data.width-32*2, 
			20,
			hwnd,  /* Parent */
			(HMENU)1,
			cs->hInstance,
			NULL
			);

		SendMessage(text, WM_SETTEXT, "test1", "test2");

		data.edit = CreateWindow (
			WC_EDIT,
			"EditPath",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
			32, 
			32,
			data.width-32*2, 
			20,
			hwnd,  /* Parent */
			(HMENU)1,
			cs->hInstance,
			NULL
			);
 
		data.installButton = CreateWindow (
			WC_BUTTON,
			"Install",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			(data.width - 80)/2,
			data.height - 32*2,
			80, 
			32,
			hwnd,  /* Parent */
			(HMENU)1,
			cs->hInstance,
			NULL
			);
		return 0;
	    }   
	    case WM_PAINT: {
		return DefWindowProc(hwnd, message, wParam, lParam);
	    }
	    case WM_COMMAND: {
		data.ok = 1;
		DestroyWindow(wnd_params);
		return;
	    }
	    case WM_KEYDOWN: {
		if(wParam == 0x49) {
		    DestroyWindow(wnd_params);
		}
		return 0;
	    }
	    case WM_DESTROY:
		if(!data.ok)
                    PostQuitMessage (0);
		wnd_params = 0;
		return DefWindowProc(hwnd, message, wParam, lParam);
	    default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void processMessages()
{
    MSG msg;
    while(PeekMessage(&msg,NULL,0,0,0))
    {
	GetMessage(&msg, NULL, 0, 0);
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
}

static char*lastmessage = 0;
void myarchivestatus(int type, char*text)
{
    if(text && text[0]=='[')
	return;
    //printf("%s\n", text);
			
    SendMessage(wnd_progress, USER_SETMESSAGE, (WPARAM)strdup(text), 0);
    SendMessage(wnd_progress, WM_PAINT, 0, 0);
    int t;
    for(t=0;t<9;t++) {
	SendMessage(wnd_progress, PBM_STEPIT, 0, 0);
	/* while we're here, we might also make ourselves useful */
	processMessages();
	/* we want the user to see what we're writing, right? */
	Sleep(20);
    }

    if(type<0) {
	while(1) {
	    int ret = MessageBox(0, text, "Error", MB_RETRYCANCEL|MB_ICONERROR);
	    
	    /* there is no MB_CANCEL, so, *sigh*, we have to display
	       the "retry" button. So pretend it's doing anything... */
	    if(ret==IDRETRY)
		continue;
	    else
		break;
	}
    }
}

static int regEnter(char*key,char*value)
{
    HKEY hkey;
    int ret = 0;
    ret = RegCreateKey(HKEY_LOCAL_MACHINE, key, &hkey);
    if(ret != ERROR_SUCCESS) {
	fprintf(stderr, "registry: CreateKey %s failed\n", key);
	return 0;
    }
    ret = RegSetValue(hkey, NULL, REG_SZ, value, strlen(value)+1);
    if(ret != ERROR_SUCCESS) {
	fprintf(stderr, "registry: SetValue %s failed\n", key);
	return 0;
    }
    return 1;
}

int addRegistryEntries(char*install_dir)
{
    int ret;
    ret = regEnter("Software\\quiss.org\\swftools\\InstallPath", install_dir);
    if(!ret) return 0;
    return 1;
}

int WINAPI WinMain(HINSTANCE me,HINSTANCE hPrevInst,LPSTR lpszArgs, int nWinMode)
{
    WNDCLASSEX wcl;
    char*install_dir = "C:\\swftools\\";

    wcl.hInstance    = me;
    wcl.lpszClassName= "SWFTools-Installer";
    wcl.lpfnWndProc  = WindowFunc;
    wcl.style        = CS_HREDRAW | CS_VREDRAW;
    wcl.hIcon        = LoadIcon(NULL, IDI_APPLICATION);
    wcl.hIconSm      = LoadIcon(NULL, IDI_APPLICATION);
    wcl.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = NULL; //no menu
    wcl.cbClsExtra   = 0;
    wcl.cbWndExtra   = 0;
    //wcl.hbrBackground= (HBRUSH) GetStockObject(DKGRAY_BRUSH);
    wcl.hbrBackground= (HBRUSH) GetStockObject (WHITE_BRUSH);
    wcl.cbSize       = sizeof(WNDCLASSEX);

    if(!RegisterClassEx (&wcl)) {
	return 0;
    }

    InitCommonControls();
   
    CreateWindow (
	    wcl.lpszClassName,          /* Class name */
	    "SWFTools Installer",            /* Caption */
	    WS_OVERLAPPEDWINDOW&(~WS_SIZEBOX),        /* Style */
	    CW_USEDEFAULT,              /* Initial x (use default) */
	    CW_USEDEFAULT,              /* Initial y (use default) */
	    320,                        /* Initial x size */
	    200,                        /* Initial y size */
	    NULL,                       /* No parent window */
	    NULL,                       /* No menu */
	    me,                         /* This program instance */
	    (void*)"params"		/* Creation parameters */
	    );
    ShowWindow (wnd_params, nWinMode);
    UpdateWindow (wnd_params);
   
    MSG msg;
    while(wnd_params)
    {
	GetMessage(&msg,NULL,0,0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
   
    char buf[1024];
    sprintf(buf, "Do you want me to install SWFTools into the directory %s now?", install_dir);
    int ret = MessageBox(0, buf, "SWFTools Install", MB_YESNO|MB_ICONQUESTION);

    if(ret == IDNO)
	return 0;
    
    CreateWindow (
	    wcl.lpszClassName,          /* Class name */
	    "Installing...",            /* Caption */
	    WS_OVERLAPPEDWINDOW&(~WS_SIZEBOX),        /* Style */
	    CW_USEDEFAULT,              /* Initial x (use default) */
	    CW_USEDEFAULT,              /* Initial y (use default) */
	    260,                        /* Initial x size */
	    128,                        /* Initial y size */
	    NULL,                       /* No parent window */
	    NULL,                       /* No menu */
	    me,                         /* This program instance */
	    (void*)"progress"		/* Creation parameters */
	    );
    ShowWindow (wnd_progress, nWinMode);
    UpdateWindow (wnd_progress);
    
    int success = unpack_archive(crndata, "C:\\swftools\\", myarchivestatus);
   
    DestroyWindow(wnd_progress);

    while(wnd_progress)
	processMessages();

    if(!addRegistryEntries(install_dir)) {
	success = 0;
	ret = MessageBox(0, "Couldn't create Registry Entries", "SWFTools Install", MB_OK|MB_ICONERROR);
    }

    if(success) {
	sprintf(buf, "SWFTools Version %s has been installed into %s successfully", VERSION, install_dir);
	ret = MessageBox(0, buf, "SWFTools Install", MB_OK|MB_ICONINFORMATION);
    } else {
	/* error will already have been notified by either myarchivestatus or some other
	   routine */
	/*sprintf(buf, "Installation failed\nLast message: %s", lastmessage);
	ret = MessageBox(0, buf, "SWFTools Install", MB_OK|MB_ICONERROR);*/
    }
    exit(0);
}



