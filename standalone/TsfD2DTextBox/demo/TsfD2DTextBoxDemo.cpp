#include <Windows.h>

#include "TsfD2DTextBox.h"

namespace
{
const wchar_t *kWindowClass = L"TsfD2DTextBoxDemoWindow";
const wchar_t *kWindowTitle = L"TsfD2DTextBox Demo";

TsfD2DTextBox *g_textBox = nullptr;

LRESULT CALLBACK DemoWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        RECT rcClient = {};
        GetClientRect(hwnd, &rcClient);

        g_textBox = new TsfD2DTextBox();
        RECT bounds = {0, 0, rcClient.right, rcClient.bottom};
        g_textBox->Create(GetModuleHandle(NULL), hwnd, bounds);
        return 0;
    }

    case WM_SHOWWINDOW:
        if (wParam && g_textBox && g_textBox->GetHwnd())
        {
            SetFocus(g_textBox->GetHwnd());
        }
        return 0;

    case WM_SETFOCUS:
        if (g_textBox && g_textBox->GetHwnd())
        {
            SetFocus(g_textBox->GetHwnd());
        }
        return 0;

    case WM_SIZE:
        if (g_textBox)
        {
            RECT rcClient = {};
            GetClientRect(hwnd, &rcClient);
            g_textBox->Move(0, 0, rcClient.right, rcClient.bottom);
        }
        return 0;

    case WM_DESTROY:
        delete g_textBox;
        g_textBox = nullptr;
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}
} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    TsfD2DTextBox::RegisterClass(hInstance);

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = DemoWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = kWindowClass;
    RegisterClassEx(&wcex);

    HWND hwnd = CreateWindow(kWindowClass, kWindowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                             960, 640, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
