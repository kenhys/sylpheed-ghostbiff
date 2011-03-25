#include <stdio.h>

#define _WIN32_WINNT 0x0500
#include <windows.h>

int main(int argc, char* argv[])
{
    DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST;
    DWORD dwStyle = WS_POPUP | WS_VISIBLE;
    LPVOID lpParam = NULL;
    int x = 0;
    int y = 0;
    int nWidth = 100;
    int nHeight = 100;
    HWND hWndParent = NULL;
    HMENU hMenu = 0;
    HINSTANCE hInstance = NULL;
    HWND hWnd = CreateWindowEx(dwExStyle,
                   "ghostbiff",
                   "BIFF",
                   dwStyle,
                   x,
                   y,
                   nWidth,
                   nHeight,
                   hWndParent,
                   hMenu,
                   hInstance,
                   lpParam);
    ShowWindow(hWnd, SW_SHOWNORMAL);
    Sleep(10000);
                   
}
