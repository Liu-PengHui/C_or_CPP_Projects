// RandNumGtr.cpp : 定义应用程序的入口点。

#include "framework.h"
#include "RandNumGtr.h"
#include <windows.h>
#include <commctrl.h>
#include <random>
#include <shellapi.h>   // 修复 ShellExecute 未定义
#include <stdlib.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

// 添加清单依赖以启用视觉样式
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100
#define REGISTRY_KEY_PATH L"Software\\RandNumGenerator"
#define WM_UPDATE_SETTINGS (WM_APP + 100)   // 自定义消息：设置已更改

// 配色方案枚举
enum ColorScheme {
    SCHEME_BLUE = 1,
    SCHEME_PINK = 2,
    SCHEME_YELLOW = 3,
    SCHEME_RED = 4
};

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hEdit1, hEdit2, hButton;
WCHAR szResult[MAX_LOADSTRING] = L"";

// 配色与透明度相关全局变量
int g_colorScheme = SCHEME_BLUE;                // 当前配色方案
BYTE g_alpha = 225;                             // 窗口透明度 (0-255)
COLORREF g_themeColor = RGB(0, 120, 215);       // 主题色（字体、边框）
COLORREF g_editBgColor = RGB(240, 248, 255);    // 输入框/结果显示框背景色
HBRUSH g_hEditBgBrush = NULL;                   // 编辑框背景画刷

// 函数声明
ATOM                MyRegisterClass(HINSTANCE);
void                CenterWindowOnNearestMonitor(HWND);
BOOL                InitInstance(HINSTANCE, int);
long long           GenerateRandomNumber(long long, long long);
void                DrawRoundRectBorder(HDC, int, int, int, int, int, int, COLORREF);
void                SaveValuesToRegistry(HWND);
void                LoadValuesFromRegistry(HWND);
void                LoadColorAndAlphaFromRegistry();   // 读取配色和透明度
void                SaveColorAndAlphaToRegistry();     // 保存配色和透明度
void                ApplySettings(HWND);                // 应用当前配色和透明度
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SettingsDialog(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RANDNUMGTR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RANDNUMGTR));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RANDNUMGTR));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = NULL;  // 重要：设置为NULL以实现透明背景
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_RANDNUMGTR);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// 窗口居中
void CenterWindowOnNearestMonitor(HWND hWnd)
{
    HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMonitor, &mi);

    RECT rcWindow;
    GetWindowRect(hWnd, &rcWindow);
    int windowWidth = rcWindow.right - rcWindow.left;
    int windowHeight = rcWindow.bottom - rcWindow.top;

    int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - windowWidth) / 2;
    int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - windowHeight) / 2;

    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, 540, 360, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // 设置窗口为分层窗口（支持透明度）
    SetWindowLong(hWnd, GWL_EXSTYLE,
        GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    dwStyle &= ~(WS_MAXIMIZEBOX | WS_SIZEBOX);
    SetWindowLong(hWnd, GWL_STYLE, dwStyle);

    // 从注册表读取配色和透明度
    LoadColorAndAlphaFromRegistry();
    // 应用配色和透明度（创建画刷、设置透明度）
    ApplySettings(hWnd);

    CenterWindowOnNearestMonitor(hWnd);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 从注册表加载保存的数值
    LoadValuesFromRegistry(hWnd);

    return TRUE;
}

// 真随机数生成器（使用 Windows BCrypt 加密 API）
long long GenerateRandomNumber(long long min, long long max)
{
    // 自动纠正范围颠倒
    if (min > max) {
        long long tmp = min;
        min = max;
        max = tmp;
    }

    // 静态初始化 BCrypt 算法句柄（只一次）
    static BCRYPT_ALG_HANDLE hAlg = NULL;
    static bool bInitOk = false;
    if (!bInitOk) {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0);
        bInitOk = (status == 0);   // STATUS_SUCCESS = 0
    }

    // 如果初始化失败或句柄无效，降级到标准库随机
    if (!bInitOk || hAlg == NULL) {
        static std::mt19937_64 gen(std::random_device{}());
        std::uniform_int_distribution<long long> distrib(min, max);
        return distrib(gen);
    }

    // 计算范围宽度
    unsigned long long range = static_cast<unsigned long long>(max - min);

    // 根据范围大小选择所需随机字节数（减少循环次数）
    int bytesNeeded = 8;
    if (range <= 0xFF)        bytesNeeded = 1;
    else if (range <= 0xFFFF) bytesNeeded = 2;
    else if (range <= 0xFFFFFFFF) bytesNeeded = 4;

    unsigned long long randomValue;
    do {
        // 生成随机字节
        NTSTATUS status = BCryptGenRandom(hAlg, (PUCHAR)&randomValue, bytesNeeded, 0);
        if (status != 0) {   // 调用失败 -> 降级
            static std::mt19937_64 fallbackGen(std::random_device{}());
            std::uniform_int_distribution<long long> fallbackDistrib(min, max);
            return fallbackDistrib(fallbackGen);
        }

        // 屏蔽多余的字节（保证值在 [0, 2^(8*bytesNeeded)-1] 范围内）
        if (bytesNeeded < 8) {
            randomValue &= ((1ULL << (bytesNeeded * 8)) - 1);
        }
    } while (randomValue > range);   // 拒绝采样，保证均匀

    return min + static_cast<long long>(randomValue);
}

// 绘制圆角矩形边框（支持自定义颜色）
void DrawRoundRectBorder(HDC hdc, int left, int top, int right, int bottom,
    int ellipseWidth, int ellipseHeight, COLORREF borderColor)
{
    HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN hPen = CreatePen(PS_SOLID, 2, borderColor);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hNullBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    RoundRect(hdc, left, top, right, bottom, ellipseWidth, ellipseHeight);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

// 保存最小值和最大值到注册表
void SaveValuesToRegistry(HWND hWnd)
{
    HKEY hKey;
    LONG lResult = RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_PATH, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

    if (lResult == ERROR_SUCCESS)
    {
        WCHAR minText[32] = { 0 };
        WCHAR maxText[32] = { 0 };

        GetWindowText(hEdit1, minText, 32);
        GetWindowText(hEdit2, maxText, 32);

        RegSetValueExW(hKey, L"MinValue", 0, REG_SZ,
            (const BYTE*)minText, (DWORD)((wcslen(minText) + 1) * sizeof(WCHAR)));

        RegSetValueExW(hKey, L"MaxValue", 0, REG_SZ,
            (const BYTE*)maxText, (DWORD)((wcslen(maxText) + 1) * sizeof(WCHAR)));

        RegCloseKey(hKey);
    }
}

// 从注册表加载最小值和最大值
void LoadValuesFromRegistry(HWND hWnd)
{
    HKEY hKey;
    LONG lResult = RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_PATH, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        WCHAR minText[32] = { 0 };
        WCHAR maxText[32] = { 0 };
        DWORD size = sizeof(minText);

        if (RegQueryValueExW(hKey, L"MinValue", NULL, NULL, (LPBYTE)minText, &size) == ERROR_SUCCESS)
        {
            SetWindowText(hEdit1, minText);
        }

        size = sizeof(maxText);
        if (RegQueryValueExW(hKey, L"MaxValue", NULL, NULL, (LPBYTE)maxText, &size) == ERROR_SUCCESS)
        {
            SetWindowText(hEdit2, maxText);
        }

        RegCloseKey(hKey);
    }
}

// 从注册表加载配色和透明度
void LoadColorAndAlphaFromRegistry()
{
    HKEY hKey;
    LONG lResult = RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_PATH, 0, KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD scheme = 1, alpha = 225;
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ColorScheme", NULL, NULL, (LPBYTE)&scheme, &size) == ERROR_SUCCESS)
        {
            g_colorScheme = scheme;
        }
        size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Alpha", NULL, NULL, (LPBYTE)&alpha, &size) == ERROR_SUCCESS)
        {
            g_alpha = (BYTE)alpha;
        }
        RegCloseKey(hKey);
    }
    else
    {
        // 默认值
        g_colorScheme = SCHEME_BLUE;
        g_alpha = 225;
    }

    // 根据配色方案设置主题色和背景色
    switch (g_colorScheme)
    {
    case SCHEME_BLUE:
        g_themeColor = RGB(0, 120, 215);
        g_editBgColor = RGB(255, 255, 255);
        break;
    case SCHEME_PINK:
        g_themeColor = RGB(255, 105, 180);
        g_editBgColor = RGB(255, 255, 255);
        break;
    case SCHEME_YELLOW:
        g_themeColor = RGB(255, 193, 37);
        g_editBgColor = RGB(255, 255, 255);
        break;
    case SCHEME_RED:
        g_themeColor = RGB(220, 20, 60);
        g_editBgColor = RGB(255, 255, 255);
        break;
    default:
        g_themeColor = RGB(0, 120, 215);
        g_editBgColor = RGB(255, 255, 255);
        break;
    }
}

// 保存配色和透明度到注册表
void SaveColorAndAlphaToRegistry()
{
    HKEY hKey;
    LONG lResult = RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY_PATH, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (lResult == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"ColorScheme", 0, REG_DWORD, (const BYTE*)&g_colorScheme, sizeof(DWORD));
        DWORD alpha = g_alpha;
        RegSetValueExW(hKey, L"Alpha", 0, REG_DWORD, (const BYTE*)&alpha, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// 应用当前配色和透明度（重建画刷、设置窗口透明度、请求重绘）
void ApplySettings(HWND hWnd)
{
    // 1. 更新编辑框背景画刷
    if (g_hEditBgBrush)
        DeleteObject(g_hEditBgBrush);
    g_hEditBgBrush = CreateSolidBrush(g_editBgColor);

    // 2. 设置窗口透明度
    SetLayeredWindowAttributes(hWnd, 0, (g_alpha*205)/100+50, LWA_ALPHA);

    // 3. 刷新所有控件和窗口（触发重绘和WM_CTLCOLOREDIT）
    if (hWnd)
    {
        InvalidateRect(hWnd, NULL, TRUE);
        RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 创建第一个编辑框（最小值）
        hEdit1 = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            113, 23, 144, 24, hWnd, (HMENU)IDC_EDIT1, hInst, NULL);

        // 创建第二个编辑框（最大值）
        hEdit2 = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            113, 73, 144, 24, hWnd, (HMENU)IDC_EDIT2, hInst, NULL);

        // 创建生成按钮
        hButton = CreateWindowW(L"BUTTON", L"生成随机数",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
            300, 20, 200, 80, hWnd, (HMENU)IDC_BUTTON, hInst, NULL);

        // 设置控件字体
        HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

        HFONT hBtnFont = CreateFont(36, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");

        SendMessage(hEdit1, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit1, EM_SETLIMITTEXT, 15, 0);
        SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit2, EM_SETLIMITTEXT, 15, 0);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    }
    break;

    case WM_ERASEBKGND:
        return 1;  // 阻止默认背景擦除

    case WM_CTLCOLOREDIT:   // 处理编辑框颜色
    {
        HDC hdcEdit = (HDC)wParam;
        HWND hwndEdit = (HWND)lParam;
        if (hwndEdit == hEdit1 || hwndEdit == hEdit2)
        {
            SetTextColor(hdcEdit, g_themeColor);
            SetBkColor(hdcEdit, g_editBgColor);
            if (g_hEditBgBrush)
                return (LRESULT)g_hEditBgBrush;
        }
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_BUTTON)
        {
            wchar_t minText[32] = { 0 };
            wchar_t maxText[32] = { 0 };

            GetWindowText(hEdit1, minText, 32);
            GetWindowText(hEdit2, maxText, 32);

            long long min = _wtoi64(minText);
            long long max = _wtoi64(maxText);

            if (min >= max)
            {
                MessageBox(hWnd, L"最大值必须大于最小值", L"输入错误", MB_ICONWARNING);
                return 0;
            }

            long long randomNum = GenerateRandomNumber(min, max);
            swprintf_s(szResult, L"%lld", randomNum);  // 注意：随机数可能为负数？实际unsigned，但保留llu可改为%lld？用%llu更好
            // 实际随机数生成使用unsigned long long，输出为无符号
            // 修正：用 %llu
            swprintf_s(szResult, L"%llu", (unsigned long long)randomNum);

            SaveValuesToRegistry(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
        }
        else
        {
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_SETTINGS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDialog);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_OPENSRC:
                ShellExecute(NULL, L"open",
                    L"https://github.com/Liu-PengHui/C_or_CPP_Projects/blob/main/RandNumGenerater3.2/src/RandNumGtr.cpp",
                    NULL, NULL, SW_SHOWNORMAL);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    }
    break;

    case WM_UPDATE_SETTINGS:   // 自定义消息：设置已更改，重新应用
    {
        // 重新加载配色和透明度（已保存在注册表中）
        LoadColorAndAlphaFromRegistry();
        ApplySettings(hWnd);
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 填充整个窗口背景为白色（经典白底）
        HBRUSH hBgBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &ps.rcPaint, hBgBrush);
        DeleteObject(hBgBrush);

        // 绘制两个输入框的外框（使用主题色）
        DrawRoundRectBorder(hdc, 110, 20, 260, 50, 10, 10, g_themeColor);
        DrawRoundRectBorder(hdc, 110, 70, 260, 100, 10, 10, g_themeColor);

        // 设置文本颜色为当前主题色，背景透明
        SetTextColor(hdc, g_themeColor);
        SetBkMode(hdc, TRANSPARENT);

        // 绘制标签“最小值”“最大值”
        HFONT hLabelFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hLabelFont);

        RECT rectMin = { 20,20,100,50 };
        DrawTextW(hdc, L"最小值", -1, &rectMin, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT rectMax = { 20,70,100,100 };
        DrawTextW(hdc, L"最大值", -1, &rectMax, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 绘制结果显示区域（背景填充编辑框背景色）
        RECT rectResult = { 30,150,500,270 };
        HBRUSH hResultBgBrush = CreateSolidBrush(g_editBgColor);
        FillRect(hdc, &rectResult, hResultBgBrush);
        DeleteObject(hResultBgBrush);
        DrawRoundRectBorder(hdc, rectResult.left, rectResult.top,
            rectResult.right, rectResult.bottom, 10, 10, g_themeColor);

        // 动态调整显示数字的字体大小
        int strLength = (int)wcslen(szResult);
        int fontSize = strLength < 10 ? 100 : max(20, 100 - strLength * 3);
        HFONT hResultFont = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        SelectObject(hdc, hResultFont);
        SetTextColor(hdc, g_themeColor);
        DrawTextW(hdc, szResult, -1, &rectResult, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 清理GDI对象
        SelectObject(hdc, hOldFont);
        DeleteObject(hLabelFont);
        DeleteObject(hResultFont);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        if (g_hEditBgBrush)
            DeleteObject(g_hEditBgBrush);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// 设置对话框回调（完整实现）
INT_PTR CALLBACK SettingsDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // 初始化滑块控件（透明度）
        HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER1);
        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hSlider, TBM_SETPOS, TRUE, g_alpha);
        SendMessage(hSlider, TBM_SETTICFREQ, 32, 0);

        // 根据当前配色选中对应的单选按钮
        switch (g_colorScheme)
        {
        case SCHEME_BLUE:
            CheckRadioButton(hDlg, IDC_RADIO_BLUE_DEFAULT, IDC_RADIO_RED, IDC_RADIO_BLUE_DEFAULT);
            break;
        case SCHEME_PINK:
            CheckRadioButton(hDlg, IDC_RADIO_BLUE_DEFAULT, IDC_RADIO_RED, IDC_RADIO_PINK);
            break;
        case SCHEME_YELLOW:
            CheckRadioButton(hDlg, IDC_RADIO_BLUE_DEFAULT, IDC_RADIO_RED, IDC_RADIO_YELLOW);
            break;
        case SCHEME_RED:
            CheckRadioButton(hDlg, IDC_RADIO_BLUE_DEFAULT, IDC_RADIO_RED, IDC_RADIO_RED);
            break;
        default:
            CheckRadioButton(hDlg, IDC_RADIO_BLUE_DEFAULT, IDC_RADIO_RED, IDC_RADIO_BLUE_DEFAULT);
            break;
        }
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            // 获取选中的配色
            int newScheme = SCHEME_BLUE;
            if (IsDlgButtonChecked(hDlg, IDC_RADIO_BLUE_DEFAULT) == BST_CHECKED)
                newScheme = SCHEME_BLUE;
            else if (IsDlgButtonChecked(hDlg, IDC_RADIO_PINK) == BST_CHECKED)
                newScheme = SCHEME_PINK;
            else if (IsDlgButtonChecked(hDlg, IDC_RADIO_YELLOW) == BST_CHECKED)
                newScheme = SCHEME_YELLOW;
            else if (IsDlgButtonChecked(hDlg, IDC_RADIO_RED) == BST_CHECKED)
                newScheme = SCHEME_RED;

            // 获取透明度
            HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER1);
            int newAlpha = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);

            // 保存到全局变量
            g_colorScheme = newScheme;
            g_alpha = (BYTE)newAlpha;
            SaveColorAndAlphaToRegistry();

            // 通知主窗口更新设置
            HWND hParent = GetParent(hDlg);
            PostMessage(hParent, WM_UPDATE_SETTINGS, 0, 0);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    }
    return (INT_PTR)FALSE;
}