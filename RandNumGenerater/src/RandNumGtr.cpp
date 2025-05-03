// RandNunGtr.cpp : 定义应用程序的入口点。

#include "framework.h"
#include "RandNumGtr.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

// 添加清单依赖以启用视觉样式
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100
#define REGISTRY_KEY_PATH L"Software\\RandNumGenerator"

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hEdit1, hEdit2, hButton;
WCHAR szResult[MAX_LOADSTRING] = L"";

// 函数声明
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                SaveValuesToRegistry(HWND hWnd);
void                LoadValuesFromRegistry(HWND hWnd);

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
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
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

    // 设置窗口为分层窗口
    SetWindowLong(hWnd, GWL_EXSTYLE,
        GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    dwStyle &= ~(WS_MAXIMIZEBOX | WS_SIZEBOX);

    SetWindowLong(hWnd, GWL_STYLE, dwStyle);

    // 设置窗口透明度
    SetLayeredWindowAttributes(hWnd, 0, 225, LWA_ALPHA);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 从注册表加载保存的值
    LoadValuesFromRegistry(hWnd);

    return TRUE;
}

long long GenerateRandomNumber(long long min, long long max)
{
    static std::mt19937_64 gen(std::random_device{}());
    std::uniform_int_distribution<long long> distrib(min, max);
    return distrib(gen);
}

void DrawRoundRectBorder(HDC hdc, int left, int top, int right, int bottom, int ellipseWidth, int ellipseHeight)
{
    // 创建透明画刷和实线画笔
    HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215)); // 蓝色边框

    // 选入设备上下文
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hNullBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // 绘制空心圆角矩形
    RoundRect(hdc, left, top, right, bottom, ellipseWidth, ellipseHeight);

    // 恢复原有对象
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

        // 保存最小值
        RegSetValueExW(hKey, L"MinValue", 0, REG_SZ,
            (const BYTE*)minText, (wcslen(minText) + 1) * sizeof(WCHAR));

        // 保存最大值
        RegSetValueExW(hKey, L"MaxValue", 0, REG_SZ,
            (const BYTE*)maxText, (wcslen(maxText) + 1) * sizeof(WCHAR));

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

        // 读取最小值
        if (RegQueryValueExW(hKey, L"MinValue", NULL, NULL, (LPBYTE)minText, &size) == ERROR_SUCCESS)
        {
            SetWindowText(hEdit1, minText);
        }

        size = sizeof(maxText);
        // 读取最大值
        if (RegQueryValueExW(hKey, L"MaxValue", NULL, NULL, (LPBYTE)maxText, &size) == ERROR_SUCCESS)
        {
            SetWindowText(hEdit2, maxText);
        }

        RegCloseKey(hKey);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 创建第一个编辑框
        hEdit1 = CreateWindowW(
            L"EDIT",                                   // 编辑框类名
            L"",                                 // 默认文本
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER, // 样式
            113, 23, 144, 24,                           // 位置和大小
            hWnd,                                      // 父窗口
            (HMENU)IDC_EDIT1,                          // 控件ID
            hInst,                                     // 实例句柄
            NULL);

        // 创建第二个编辑框
        hEdit2 = CreateWindowW(
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            113, 73, 144, 24,
            hWnd,
            (HMENU)IDC_EDIT2,
            hInst,
            NULL);

        // 创建生成按钮
        hButton = CreateWindowW(
            L"BUTTON",                                 // 按钮类名
            L"生成随机数",                              // 按钮文本
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, // 样式
            300, 20, 200, 80,                          // 位置和大小
            hWnd,
            (HMENU)IDC_BUTTON,
            hInst,
            NULL);

        // 设置控件字体
        HFONT hFont = CreateFont(
            20,                           // 字体高度（原来为默认值约16）
            0,                            // 宽度（0表示自动）
            0,                            // 旋转角度
            0,                            // 方向
            FW_NORMAL,                    // 字体粗细
            FALSE,                        // 斜体
            FALSE,                        // 下划线
            FALSE,                        // 删除线
            DEFAULT_CHARSET,              // 字符集
            OUT_DEFAULT_PRECIS,           // 输出精度
            CLIP_DEFAULT_PRECIS,          // 剪裁精度
            CLEARTYPE_QUALITY,            // 输出质量（清晰度）
            DEFAULT_PITCH | FF_SWISS,     // 间距和字体族
            L"Microsoft YaHei"            // 字体名称（使用微软雅黑）
        );

        HFONT hBtnFont = CreateFont(
            36,                           // 字体高度（原来为默认值约16）
            0,                            // 宽度（0表示自动）
            0,                            // 旋转角度
            0,                            // 方向
            FW_NORMAL,                    // 字体粗细
            FALSE,                        // 斜体
            FALSE,                        // 下划线
            FALSE,                        // 删除线
            DEFAULT_CHARSET,              // 字符集
            OUT_DEFAULT_PRECIS,           // 输出精度
            CLIP_DEFAULT_PRECIS,          // 剪裁精度
            CLEARTYPE_QUALITY,            // 输出质量（清晰度）
            DEFAULT_PITCH | FF_SWISS,     // 间距和字体族
            L"Arial"            // 字体名称（使用微软雅黑）
        );

        SendMessage(hEdit1, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit1, EM_SETLIMITTEXT, 15, 0);
        SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit2, EM_SETLIMITTEXT, 15, 0);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    }
    break;

    case WM_ERASEBKGND:
        return 1;  // 阻止默认背景擦除

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        if (wmId == IDC_BUTTON)
        {
            wchar_t minText[32] = { 0 };
            wchar_t maxText[32] = { 0 };

            GetWindowText(hEdit1, minText, 32);
            GetWindowText(hEdit2, maxText, 32);

            unsigned long long min = _wtoi64(minText);
            unsigned long long max = _wtoi64(maxText);

            // 验证输入
            if (min >= max)
            {
                MessageBox(hWnd, L"最大值必须大于最小值", L"输入错误", MB_ICONWARNING);
                return 0;
            }

            // 生成随机数
            unsigned long long randomNum = GenerateRandomNumber(min, max);

            swprintf_s(szResult, L"%llu", randomNum);

            // 保存当前的最小值和最大值到注册表
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
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_OPENSRC:
                ShellExecute(NULL, L"open", 
                    L"https://github.com/Liu-PengHui/C_or_CPP_Projects/RandNumGenerater/src/RandNumGtr.cpp", 
                    NULL, NULL, SW_SHOWNORMAL);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 创建带透明度的画刷
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);
        DrawRoundRectBorder(hdc, 110, 20, 260, 50, 10, 10);
        DrawRoundRectBorder(hdc, 110, 70, 260, 100, 10, 10);

        // 设置文本
        SetTextColor(hdc, RGB(0, 120, 215));
        SetBkMode(hdc, TRANSPARENT);

        HFONT hFont = CreateFont(
            24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        RECT rectMin = { 20,20,100,50 };
        DrawTextW(hdc, L"最小值", -1, &rectMin,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT rectMax = { 20,70,100,100 };
        DrawTextW(hdc, L"最大值", -1, &rectMax,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        int strLength = (int)wcslen(szResult);

        hFont = CreateFont(
            strLength < 10 ? 100 : 100 - strLength * 3, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        hOldFont = (HFONT)SelectObject(hdc, hFont);

        RECT rectResult = { 30,150,500,270 };
        DrawRoundRectBorder(hdc, rectResult.left, rectResult.top,
            rectResult.right, rectResult.bottom, 10, 10);
        DrawTextW(hdc, szResult, -1, &rectResult,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
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