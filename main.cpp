#include <windows.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <time.h>
#include <tchar.h>
#include <Psapi.h>
#include <Oleacc.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "Oleacc.lib" )

#define COPY_KEY 67
#define MAX_DRIVES 256
#define MAX_CAMERAS 5

using namespace std;
using namespace cv;

HWINEVENTHOOK hWebBrowserHook;
HHOOK hKeyboardHook, hRMouseHook, hUSBdetectHook, hCameradetectHook;
ofstream fout;
string str = "user_operation.txt";
wstring wndTitle;
int usbNum, camNum;

void InterateMenu(HMENU hMenu)
{
	MENUITEMINFO mii;
	int i, nCount = GetMenuItemCount(hMenu);

	for (i = 0; i < nCount; i++)
	{
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID; // | MIIM_STATE
		if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
			continue;
		if ((mii.fType & MFT_STRING) != 0 && mii.cch > 0)
		{
			mii.cch++;
			TCHAR* pString = (TCHAR*)malloc(mii.cch * sizeof(TCHAR));
			if (pString != NULL)
			{
				if (!GetMenuItemInfo(hMenu, i, TRUE, &mii))
				{
					free(pString);
					continue;
				}
				MessageBox(NULL, pString, L"Camera hook", MB_OK);
				//TRACE(_T("ID = %u, string = %s\n"), mii.wID, pString);
				free(pString);
			}
		}
		if (mii.hSubMenu != NULL)
			InterateMenu(mii.hSubMenu); // ** recursive **
	}
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	DWORD SHIFT_key = 0;
	DWORD CTRL_key = 0;
	DWORD ALT_key = 0;

	if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
	{
		KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);
		DWORD dwMsg = 1;
		dwMsg += hooked_key.scanCode << 16;
		dwMsg += hooked_key.flags << 24;
		wchar_t lpszKeyName[1024] = { 0 };
		lpszKeyName[0] = '[';
				
		int i = GetKeyNameText(dwMsg, (lpszKeyName + 1), 0xFF) + 1;
		lpszKeyName[i] = ']';

		int key = hooked_key.vkCode;

		SHIFT_key = GetAsyncKeyState(VK_SHIFT);
		CTRL_key = GetAsyncKeyState(VK_CONTROL);
		ALT_key = GetAsyncKeyState(VK_MENU);

		struct tm localTime;
		time_t currentTime;		

		time(&currentTime);
		localtime_s(&localTime , &currentTime);

		int Hour = localTime.tm_hour;
		int Min = localTime.tm_min;
		int Sec = localTime.tm_sec;

		if (CTRL_key && key == COPY_KEY)
		{
			MessageBox(NULL, L"You tried to copy something", L"Keyboard hook", MB_OK);			

			fout <<"You tried to copy something in " << Hour << ":" << Min << ":" << Sec << endl;
				return 1;
		}

		if (key == VK_SNAPSHOT)
		{
			MessageBox(NULL, L"You tried screen capture!", L"Keyboard hook", MB_OK);
			fout << "You tried screen capture in " << Hour << ":" << Min << ":" << Sec << endl;
			return 1;
		}
		/*
		if (key == VK_F12)
		{
			MessageBox(NULL, L"You tried to press F12!", L"Keyboard hook", MB_OK);
			return 1;
		}
		*/
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	struct tm localTime;
	time_t currentTime;

	time(&currentTime);
	localtime_s(&localTime, &currentTime);

	int Hour = localTime.tm_hour;
	int Min = localTime.tm_min;
	int Sec = localTime.tm_sec;

	if (nCode == HC_ACTION)
	{
		if (wParam == WM_RBUTTONDOWN)
		{
			MessageBox(NULL, L"You tried to click right button", L"Mouse hook", MB_OK);
			fout << "You tried to click right button in " << Hour << ":" << Min << ":" << Sec << endl;
			return 1;
		}
		
	}
	return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK USBdetectProc()
{
	int k;
	wchar_t  drives[MAX_DRIVES];
	wchar_t* temp = drives;

	if (GetLogicalDriveStringsW(MAX_DRIVES, drives) == 0)
	{
		usbNum = 0;
		//return CallNextHookEx(0, nCode, wParam, lParam);
		return 0;
	}

	k = 0;
	while (*temp != NULL)
	{
		if (GetDriveTypeW(temp) == 2) k++;
		
		// Go to the next drive
		temp += lstrlenW(temp) + 1;
	}

	if (k>0) {

		if (k > usbNum)
		{
			usbNum = k;
			struct tm localTime;
			time_t currentTime;

			time(&currentTime);
			localtime_s(&localTime, &currentTime);

			int Hour = localTime.tm_hour;
			int Min = localTime.tm_min;
			int Sec = localTime.tm_sec;

			MessageBox(NULL, L"USB device was connected", L"USB hook", MB_OK);
			fout << "You connected USB device in " << Hour << ":" << Min << ":" << Sec << endl;
		}
		else
		{
			usbNum = k;
		}		
		return 1;
	}
	else
	{
		usbNum = 0;
		return 0;
	}	
}

LRESULT CALLBACK CameradetectProc()
{
	int i, k;
	k = 0;
	for (i = 0; i < MAX_CAMERAS; i++)
	{
		VideoCapture capture(i);
		if (capture.isOpened()) k++;
	}

	if (k>0)
	{
		if (k>camNum)
		{
			camNum = k;

			struct tm localTime;
			time_t currentTime;

			time(&currentTime);
			localtime_s(&localTime, &currentTime);

			int Hour = localTime.tm_hour;
			int Min = localTime.tm_min;
			int Sec = localTime.tm_sec;

			MessageBox(NULL, L"Camera was connected", L"Camera hook", MB_OK);
			fout << "You connected camera in " << Hour << ":" << Min << ":" << Sec << endl;
			
		}
		else
		{
			camNum = k;
		}
		return 1;
	}
	else
	{
		camNum = 0;
		return 0;
	}	
}

LRESULT CALLBACK WebBrowserProc()
{
	HWND foreground = GetForegroundWindow();
	if (foreground)
	{
		wchar_t window_title[256];
		GetWindowText(foreground, window_title, 256);
		wstring wst = window_title;
		if (wst != wndTitle)
		{
			//string st(wst.begin(), wst.end());
			MessageBox(NULL, window_title, L"USB hook", MB_OK);
			wndTitle = wst;
			HMENU hMenu = GetMenu(foreground);
			if(hMenu==NULL)
				MessageBox(NULL, L"NULL", L"USB hook", MB_OK);
			else
				MessageBox(NULL, L"OK", L"USB hook", MB_OK);
			InterateMenu(hMenu);
		}		
		/*
		DWORD dwPID;
		GetWindowThreadProcessId(foreground, &dwPID);

		HANDLE Handle = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			dwPID
		);

		if (Handle)
		{
			Handle = Handle;
		}
		*/
		return 1;
	}
	return 0;
}

void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (event == EVENT_SYSTEM_MENUSTART)
	{
		IAccessible* pAcc = NULL;
		VARIANT varChild;
		HRESULT hr = AccessibleObjectFromEvent(hwnd, idObject, idChild, &pAcc, &varChild);
		if ((hr == S_OK) && (pAcc != NULL))
		{
			BSTR bstrName;
			pAcc->get_accName(varChild, &bstrName);

			struct tm localTime;
			time_t currentTime;

			time(&currentTime);
			localtime_s(&localTime, &currentTime);

			int Hour = localTime.tm_hour;
			int Min = localTime.tm_min;
			int Sec = localTime.tm_sec;
			MessageBox(NULL, L"Menu was pressed.", L"Menu hook", MB_OK);
			fout << "Menu Item was pressed in " << Hour << ":" << Min << ":" << Sec << endl;
			SysFreeString(bstrName);
			pAcc->Release();
		}
	}	
}

void MessageLoop()
{
	MSG message;

	while (GetMessage(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	
}

DWORD WINAPI my_HotKey(LPVOID lpParm)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (!hInstance) hInstance = LoadLibrary((LPCWSTR)lpParm);
	if (!hInstance) return 1;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, hInstance, NULL);
	
	MessageLoop();
	UnhookWindowsHookEx(hKeyboardHook);
	return 0;
}

DWORD WINAPI my_HotMouse(LPVOID lpParm)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (!hInstance) hInstance = LoadLibrary((LPCWSTR)lpParm);
	if (!hInstance) return 1;

	hRMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)LowLevelMouseProc, hInstance, NULL);

	MessageLoop();
	UnhookWindowsHookEx(hRMouseHook);
	return 0;
}

DWORD WINAPI my_USBdetect(LPVOID lpParm)
{
	while (1)
	{
		USBdetectProc();
	}
	return 0;
}

DWORD WINAPI my_Cameradetect(LPVOID lpParm)
{
	while (1)
	{
		CameradetectProc();
	}
	return 0;
}

DWORD WINAPI my_WebBrowser(LPVOID lpParm)
{
	hWebBrowserHook = SetWinEventHook(
		EVENT_SYSTEM_MENUSTART, EVENT_SYSTEM_MOVESIZEEND,  // Range of events (4 to 5).
		NULL,                                          // Handle to DLL.
		HandleWinEvent,                                // The callback.
		0, 0,              // Process and thread IDs of interest (0 = all)
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // Flags.
	
	MessageLoop();
	UnhookWinEvent(hWebBrowserHook);
	return 0;
}

int main(int argc, char** argv)
{	
	fout.open(str);	
	usbNum = 0;
	camNum = 0;
	wndTitle = L"";
	HANDLE hKeyboardThread, hRMouseThread, hUSBdetectThread, hCameradetectThread, hWebBrowserThread;
	DWORD dwKeyboardThread, dwRMouseThread, dwUSBdetectThread, dwCameradetectThread, dwWebBrowserThread;

	hKeyboardThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotKey, (LPVOID)LowLevelKeyboardProc, NULL, &dwKeyboardThread);
	hRMouseThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotMouse, (LPVOID)LowLevelMouseProc, NULL, &dwRMouseThread);
	hUSBdetectThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_USBdetect, (LPVOID)USBdetectProc, NULL, &dwUSBdetectThread);
	hCameradetectThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_Cameradetect, (LPVOID)CameradetectProc, NULL, &dwCameradetectThread);
	hWebBrowserThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_WebBrowser, (LPVOID)WebBrowserProc, NULL, &dwWebBrowserThread);
	
	if (hKeyboardThread) return WaitForSingleObject(hKeyboardThread, INFINITE);
	else return 1;
	if (hRMouseThread) return WaitForSingleObject(hRMouseThread, INFINITE);
	else return 1;
	if (hUSBdetectThread) return WaitForSingleObject(hUSBdetectThread, INFINITE);
	else return 1;
	if (hCameradetectThread) return WaitForSingleObject(hCameradetectThread, INFINITE);
	else return 1;	
	if (hWebBrowserThread) return WaitForSingleObject(hWebBrowserThread, INFINITE);
	else return 1;
}