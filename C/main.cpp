#include <windows.h>
#include <commctrl.h>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include "rapidjson/document.h"

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "Shell32.lib")

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddJsonToTreeView(const rapidjson::GenericValue<rapidjson::UTF16<>> &value, HTREEITEM parent, HWND hwndTreeView);
HTREEITEM AddItemToTreeView(HWND hwndTV, const wchar_t *text, HTREEITEM hParent);
std::wstring GetItemPath(HWND hwndTV, HTREEITEM hItem);

#define ID_TREEVIEW 1
#define ID_OK_BUTTON 2
#define IDI_ICON 101

int main()
{
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWNORMAL);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Check if there are command line arguments and set the window title
    std::wstring windowTitle = L"TreeSelect"; // Default title
    {
        // Parse the command line into arguments
        int argc; // Argument count
        LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);
        if (argc > 1)
        {
            // Concatenate all arguments to form the title
            windowTitle.clear();
            for (int i = 1; i < argc; ++i)
            {
                if (i > 1)
                    windowTitle += L" ";
                windowTitle += argv[i];
            }
        }
        LocalFree(argv);
    }

    // Register the window class
    const wchar_t CLASS_NAME[] = L"TreeSelectWindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_ICON),
        IMAGE_ICON,
        0, 0,
        NULL));
    RegisterClassW(&wc);

    // Create the window with the determined title
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 400,
        nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndTreeView;
    static HWND hwndOkButton;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create TreeView control
        hwndTreeView = CreateWindowExW(0, WC_TREEVIEWW, L"",
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                                       0, 0, 300, 350, hwnd, (HMENU)ID_TREEVIEW, GetModuleHandle(nullptr), nullptr);

        // Create OK Button (disabled by default)
        hwndOkButton = CreateWindowExW(0, L"BUTTON", L"OK",
                           WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                           0, 350, 300, 40, hwnd, (HMENU)ID_OK_BUTTON, GetModuleHandle(nullptr), nullptr);
        EnableWindow(hwndOkButton, FALSE);

        // Read JSON from stdin
        std::wostringstream jsonStream;
        jsonStream << std::wcin.rdbuf();
        std::wstring json = jsonStream.str();

        rapidjson::GenericDocument<rapidjson::UTF16<>> doc;
        doc.Parse(json.c_str());

        // Populate TreeView with JSON
        AddJsonToTreeView(doc, TVI_ROOT, hwndTreeView);
        break;
    }
    case WM_SIZE:
    {
        // Resize TreeView control
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        MoveWindow(hwndTreeView, 0, 0, width, height - 40, TRUE);

        // Resize OK Button
        MoveWindow(hwndOkButton, 0, height - 40, width, 40, TRUE);
        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->idFrom == ID_TREEVIEW && lpnmh->code == TVN_SELCHANGEDW)
        {
            // Handle Tree View Selection Changes
            LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)lParam;
            HTREEITEM hSelectedItem = TreeView_GetSelection(hwndTreeView);

            // Enable/Disable OK Button Based on Selection
            if (hSelectedItem != NULL)
            {
                // An item is selected, enable the OK button
                EnableWindow(hwndOkButton, TRUE);
            }
            else
            {
                // No item is selected, disable the OK button
                EnableWindow(hwndOkButton, FALSE);
            }
        }
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case ID_OK_BUTTON:
        {
            // Handle OK button click
            HTREEITEM hItem = TreeView_GetSelection(hwndTreeView);
            std::wstring path = GetItemPath(hwndTreeView, hItem);

            // Print path to stdout
            std::wcout << path;

            // Close the window
            DestroyWindow(hwnd);
            break;
        }
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void AddJsonToTreeView(const rapidjson::GenericValue<rapidjson::UTF16<>> &value, HTREEITEM parent, HWND hwndTreeView)
{
    if (value.IsObject())
    {
        for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
        {
            std::wstring key = std::wstring(it->name.GetString(), it->name.GetString() + it->name.GetStringLength());
            HTREEITEM hItem = AddItemToTreeView(hwndTreeView, key.c_str(), parent);
            AddJsonToTreeView(it->value, hItem, hwndTreeView);
        }
    }
}

HTREEITEM AddItemToTreeView(HWND hwndTV, const wchar_t *text, HTREEITEM hParent)
{
    TVITEM tvi = {};
    tvi.mask = TVIF_TEXT;
    tvi.pszText = (LPTSTR)text;
    tvi.cchTextMax = wcslen(text);

    TVINSERTSTRUCT tvins = {};
    tvins.item = tvi;
    tvins.hInsertAfter = TVI_LAST;
    tvins.hParent = hParent;

    return (HTREEITEM)SendMessageW(hwndTV, TVM_INSERTITEMW, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
}

// Utility function to get the full path of an item in the tree view
std::wstring GetItemPath(HWND hwndTV, HTREEITEM hItem)
{
    std::wstring path;
    WCHAR buffer[256]; // Assuming individual item text won't exceed 255 characters

    while (hItem != NULL)
    {
        TVITEMW tvi = {};
        tvi.mask = TVIF_TEXT;
        tvi.hItem = hItem;
        tvi.pszText = buffer;
        tvi.cchTextMax = sizeof(buffer) / sizeof(buffer[0]);
        SendMessageW(hwndTV, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&tvi));

        std::wstring currentText = tvi.pszText;
        if (path.empty())
        {
            path = currentText;
        }
        else
        {
            path = currentText + L"/" + path;
        }

        hItem = TreeView_GetParent(hwndTV, hItem);
    }

    return path;
}
