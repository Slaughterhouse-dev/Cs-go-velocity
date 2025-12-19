#include <Windows.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <cmath>
#include <cstdio>
#include <Psapi.h>
#include <vector>
#include "MinHook.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "libMinHook.x86.lib")

namespace netvars {
    uintptr_t m_vecVelocity = 0x114;
}

uintptr_t clientBase = 0;
uintptr_t clientSize = 0;
uintptr_t dwLocalPlayer = 0;

float currentSpeed = 0.0f;
float lastSpeed = 0.0f;
float maxSpeed = 0.0f;
bool wasMoving = false;

HWND gameWnd = NULL;
HWND overlayWnd = NULL;
HFONT hFont = NULL;
HMODULE hModule = NULL;

float textPosX = 0.5f;
float textPosY = 0.65f;
int textWidth = 200;
int textHeight = 60;
int fontSize = 48;

// Edit mode: 0=off, 1=move, 2=resize
int editMode = 0;
bool isDragging = false;
bool isResizing = false;
POINT dragStart = { 0, 0 };
float dragStartPosX = 0;
float dragStartPosY = 0;
int dragStartW = 0;
int dragStartH = 0;

typedef BOOL(WINAPI* SetCursorPos_t)(int X, int Y);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT* lpRect);
typedef int(WINAPI* ShowCursor_t)(BOOL bShow);
typedef UINT(WINAPI* GetRawInputData_t)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);

SetCursorPos_t oSetCursorPos = nullptr;
ClipCursor_t oClipCursor = nullptr;
ShowCursor_t oShowCursor = nullptr;
GetRawInputData_t oGetRawInputData = nullptr;

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (editMode > 0) {
        return TRUE;
    }
    return oSetCursorPos(X, Y);
}

BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    if (editMode > 0) {
        return oClipCursor(NULL);
    }
    return oClipCursor(lpRect);
}

int WINAPI hkShowCursor(BOOL bShow) {
    if (editMode > 0) {
        return oShowCursor(TRUE);
    }
    return oShowCursor(bShow);
}

UINT WINAPI hkGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
    if (editMode > 0 && pData != nullptr) {
        UINT result = oGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
        if (result != (UINT)-1 && result > 0) {
            RAWINPUT* raw = (RAWINPUT*)pData;
            if (raw->header.dwType == RIM_TYPEMOUSE) {
                ZeroMemory(&raw->data.mouse, sizeof(raw->data.mouse));
            }
            if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                if (raw->data.keyboard.VKey != VK_END && 
                    raw->data.keyboard.VKey != VK_HOME &&
                    raw->data.keyboard.VKey != VK_DELETE) {
                    ZeroMemory(&raw->data.keyboard, sizeof(raw->data.keyboard));
                }
            }
        }
        return result;
    }
    return oGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
}

typedef BOOL(WINAPI* D3D9ShowCursor_t)(IDirect3DDevice9* pDevice, BOOL bShow);
D3D9ShowCursor_t oD3D9ShowCursor = nullptr;

BOOL WINAPI hkD3D9ShowCursor(IDirect3DDevice9* pDevice, BOOL bShow) {
    if (editMode > 0) {
        return oD3D9ShowCursor(pDevice, TRUE);
    }
    return oD3D9ShowCursor(pDevice, bShow);
}

IDirect3DDevice9* GetD3D9Device() {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return nullptr;
    
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetDesktopWindow();
    
    IDirect3DDevice9* pDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, 
        d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    
    pD3D->Release();
    return pDevice;
}

bool InitD3D9Hook() {
    IDirect3DDevice9* pDevice = GetD3D9Device();
    if (!pDevice) {
        printf("Failed to create D3D9 device for hook\n");
        return false;
    }
    
    void** vtable = *(void***)pDevice;
    void* pShowCursor = vtable[36];
    
    pDevice->Release();
    
    if (MH_CreateHook(pShowCursor, &hkD3D9ShowCursor, (LPVOID*)&oD3D9ShowCursor) != MH_OK) {
        printf("Failed to hook D3D9 ShowCursor\n");
        return false;
    }
    
    if (MH_EnableHook(pShowCursor) != MH_OK) {
        printf("Failed to enable D3D9 ShowCursor hook\n");
        return false;
    }
    
    printf("D3D9 ShowCursor hooked!\n");
    return true;
}

bool InitMouseHooks() {
    if (MH_Initialize() != MH_OK) {
        printf("MinHook init failed!\n");
        return false;
    }
    
    if (MH_CreateHookApi(L"user32", "SetCursorPos", &hkSetCursorPos, (LPVOID*)&oSetCursorPos) != MH_OK) {
        printf("Failed to hook SetCursorPos\n");
        return false;
    }
    
    if (MH_CreateHookApi(L"user32", "ClipCursor", &hkClipCursor, (LPVOID*)&oClipCursor) != MH_OK) {
        printf("Failed to hook ClipCursor\n");
        return false;
    }
    
    if (MH_CreateHookApi(L"user32", "ShowCursor", &hkShowCursor, (LPVOID*)&oShowCursor) != MH_OK) {
        printf("Failed to hook ShowCursor\n");
        return false;
    }
    
    if (MH_CreateHookApi(L"user32", "GetRawInputData", &hkGetRawInputData, (LPVOID*)&oGetRawInputData) != MH_OK) {
        printf("Failed to hook GetRawInputData\n");
        return false;
    }
    
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        printf("Failed to enable hooks\n");
        return false;
    }
    
    printf("Mouse hooks installed!\n");
    
    InitD3D9Hook();
    
    return true;
}

void RemoveMouseHooks() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

uintptr_t GetModuleInfo(const wchar_t* moduleName, uintptr_t* size) {
    HMODULE hMod = GetModuleHandleW(moduleName);
    if (!hMod) return 0;
    MODULEINFO modInfo;
    if (GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo))) {
        if (size) *size = modInfo.SizeOfImage;
        return (uintptr_t)modInfo.lpBaseOfDll;
    }
    return 0;
}

template<typename T>
T Read(uintptr_t address) {
    if (address == 0) return T();
    __try { return *(T*)address; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return T(); }
}

uintptr_t PatternScan(uintptr_t base, uintptr_t size, const char* pattern) {
    std::vector<BYTE> bytes;
    std::vector<bool> mask;
    
    const char* p = pattern;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        if (*p == '?') {
            bytes.push_back(0);
            mask.push_back(false);
            p++;
            if (*p == '?') p++;
        } else {
            char hex[3] = { p[0], p[1], 0 };
            bytes.push_back((BYTE)strtol(hex, nullptr, 16));
            mask.push_back(true);
            p += 2;
        }
    }
    
    BYTE* mem = (BYTE*)base;
    for (uintptr_t i = 0; i < size - bytes.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < bytes.size(); j++) {
            if (mask[j] && mem[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}

uintptr_t FindLocalPlayer() {
    const char* pattern = "8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF";
    uintptr_t addr = PatternScan(clientBase, clientSize, pattern);
    if (!addr) return 0;
    uintptr_t ptr = *(uintptr_t*)(addr + 3);
    return (ptr - clientBase) + 4;
}

uintptr_t GetLocalPlayer() {
    if (!dwLocalPlayer) return 0;
    return Read<uintptr_t>(clientBase + dwLocalPlayer);
}

float GetVelocity() {
    uintptr_t lp = GetLocalPlayer();
    if (!lp) return 0.0f;
    float vx = Read<float>(lp + netvars::m_vecVelocity);
    float vy = Read<float>(lp + netvars::m_vecVelocity + 4);
    float speed = sqrtf(vx * vx + vy * vy);
    if (speed > 5000 || isnan(speed)) return 0.0f;
    return speed;
}

RECT GetTextRect(RECT* gameRect) {
    int centerX = (int)(gameRect->right * textPosX);
    int centerY = (int)(gameRect->bottom * textPosY);
    
    RECT r;
    r.left = centerX - textWidth / 2;
    r.top = centerY - textHeight / 2;
    r.right = r.left + textWidth;
    r.bottom = r.top + textHeight;
    return r;
}

bool IsInTextArea(int x, int y, RECT* gameRect) {
    RECT tr = GetTextRect(gameRect);
    int pad = 10;
    return x >= tr.left - pad && x <= tr.right + pad && y >= tr.top - pad && y <= tr.bottom + pad;
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdcScreen = BeginPaint(hwnd, &ps);
        
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // Double buffering
            HDC hdc = CreateCompatibleDC(hdcScreen);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, rc.right, rc.bottom);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
            
            // Fill with transparent color (black = colorkey)
            HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);
            
            RECT textRect = GetTextRect(&rc);
        
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, hFont);
        
            char text[64];
            sprintf_s(text, "%d (%d)", (int)currentSpeed, (int)lastSpeed);
        
            SIZE sz;
            GetTextExtentPoint32A(hdc, text, (int)strlen(text), &sz);
            int x = textRect.left + (textWidth - sz.cx) / 2;
            int y = textRect.top + (textHeight - sz.cy) / 2;
        
            if (editMode > 0) {
                HBRUSH fillBrush = CreateSolidBrush(RGB(15, 18, 25));
                FillRect(hdc, &textRect, fillBrush);
                DeleteObject(fillBrush);
                
                HRGN clipRgn = CreateRectRgn(textRect.left, textRect.top, textRect.right, textRect.bottom);
                SelectClipRgn(hdc, clipRgn);
                DeleteObject(clipRgn);
            }
            
            SetTextColor(hdc, RGB(0, 60, 0));
            TextOutA(hdc, x + 2, y + 2, text, (int)strlen(text));
        
            SetTextColor(hdc, RGB(0, 255, 0));
            TextOutA(hdc, x, y, text, (int)strlen(text));
            
            if (editMode > 0) {
                SelectClipRgn(hdc, NULL);
                
                COLORREF borderColor = (editMode == 1) ? RGB(255, 180, 50) : RGB(50, 180, 255);
                
                HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, textRect.left, textRect.top, textRect.right, textRect.bottom);
                SelectObject(hdc, oldPen);
                SelectObject(hdc, oldBrush);
                DeleteObject(pen);
                
                if (editMode == 2) {
                    HBRUSH dotBrush = CreateSolidBrush(borderColor);
                    RECT d1 = { textRect.right - 5, textRect.bottom - 5, textRect.right - 2, textRect.bottom - 2 };
                    RECT d2 = { textRect.right - 9, textRect.bottom - 5, textRect.right - 6, textRect.bottom - 2 };
                    RECT d3 = { textRect.right - 5, textRect.bottom - 9, textRect.right - 2, textRect.bottom - 6 };
                    FillRect(hdc, &d1, dotBrush);
                    FillRect(hdc, &d2, dotBrush);
                    FillRect(hdc, &d3, dotBrush);
                    DeleteObject(dotBrush);
                }
                
                HFONT labelFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
                HFONT oldFont = (HFONT)SelectObject(hdc, labelFont);
                const char* modeText = (editMode == 1) ? "Move" : "Resize";
                SetTextColor(hdc, borderColor);
                TextOutA(hdc, textRect.left, textRect.top - 18, modeText, (int)strlen(modeText));
                SelectObject(hdc, oldFont);
                DeleteObject(labelFont);
            }
            
            BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            
            SelectObject(hdc, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdc);
        
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1;
        
        case WM_LBUTTONDOWN: {
            if (editMode == 0) break;
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            int mx = LOWORD(lParam);
            int my = HIWORD(lParam);
            
            if (!IsInTextArea(mx, my, &rc)) break;
            
            GetCursorPos(&dragStart);
            
            if (editMode == 1) {
                isDragging = true;
                dragStartPosX = textPosX;
                dragStartPosY = textPosY;
            } else if (editMode == 2) {
                isResizing = true;
                dragStartW = textWidth;
                dragStartH = textHeight;
            }
            SetCapture(hwnd);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (editMode == 0) break;
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            if (isDragging && editMode == 1) {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - dragStart.x;
                int dy = pt.y - dragStart.y;
                
                textPosX = dragStartPosX + (float)dx / rc.right;
                textPosY = dragStartPosY + (float)dy / rc.bottom;
                
                if (textPosX < 0.1f) textPosX = 0.1f;
                if (textPosX > 0.9f) textPosX = 0.9f;
                if (textPosY < 0.1f) textPosY = 0.1f;
                if (textPosY > 0.9f) textPosY = 0.9f;
                
                InvalidateRect(hwnd, NULL, FALSE);
            }
            
            if (isResizing && editMode == 2) {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - dragStart.x;
                int dy = pt.y - dragStart.y;
                
                textWidth = dragStartW + dx;
                textHeight = dragStartH + dy;
                
                if (textWidth < 100) textWidth = 100;
                if (textHeight < 40) textHeight = 40;
                if (textWidth > 500) textWidth = 500;
                if (textHeight > 200) textHeight = 200;
                
                fontSize = max(16, textHeight - 12);
                if (hFont) DeleteObject(hFont);
                hFont = CreateFontA(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            isDragging = false;
            isResizing = false;
            ReleaseCapture();
            return 0;
        }
        
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void CreateOverlay() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = hModule;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = "VelocityInternal";
    RegisterClassExA(&wc);
    
    RECT r;
    GetClientRect(gameWnd, &r);
    POINT pt = { 0, 0 };
    ClientToScreen(gameWnd, &pt);
    
    overlayWnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "VelocityInternal", NULL, WS_POPUP,
        pt.x, pt.y, r.right, r.bottom,
        NULL, NULL, hModule, NULL
    );
    
    SetLayeredWindowAttributes(overlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    hFont = CreateFontA(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial"
    );
    
    ShowWindow(overlayWnd, SW_SHOW);
}

void UpdateOverlay() {
    if (!gameWnd || !overlayWnd) return;
    
    RECT r;
    GetClientRect(gameWnd, &r);
    POINT pt = { 0, 0 };
    ClientToScreen(gameWnd, &pt);
    
    SetWindowPos(overlayWnd, HWND_TOPMOST, pt.x, pt.y, r.right, r.bottom, SWP_NOACTIVATE);
    InvalidateRect(overlayWnd, NULL, FALSE);
}

void SetEditMode(int mode) {
    if (isDragging || isResizing) {
        isDragging = false;
        isResizing = false;
        ReleaseCapture();
    }
    
    editMode = mode;
    
    LONG style = GetWindowLong(overlayWnd, GWL_EXSTYLE);
    if (mode > 0) {
        style &= ~WS_EX_TRANSPARENT;
        SetWindowLong(overlayWnd, GWL_EXSTYLE, style);
        
        oClipCursor(NULL);
        
        while (oShowCursor(TRUE) < 0);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        
    } else {
        style |= WS_EX_TRANSPARENT;
        SetWindowLong(overlayWnd, GWL_EXSTYLE, style);
    }
    
    InvalidateRect(overlayWnd, NULL, FALSE);
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    while (!(clientBase = GetModuleInfo(L"client.dll", &clientSize))) {
        Sleep(100);
    }

    Sleep(500);
    
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    printf("=== Velocity Internal ===\n");
    printf("client.dll: 0x%IX\n", clientBase);
    
    dwLocalPlayer = FindLocalPlayer();
    if (dwLocalPlayer) {
        printf("dwLocalPlayer: 0x%X\n", (unsigned)dwLocalPlayer);
    } else {
        printf("Pattern not found!\n");
    }
    
    while (!(gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9"))) {
        gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive");
        if (gameWnd) break;
        Sleep(100);
    }
    printf("Game window found!\n");
    
    if (!InitMouseHooks()) {
        printf("WARNING: Mouse hooks failed - edit mode may not work in-game\n");
    }
    
    CreateOverlay();
    printf("Overlay created!\n");
    printf("\nHOME = hide/show console\n");
    printf("DELETE = hide/show overlay\n");
    printf("END = edit mode (OFF -> MOVE -> RESIZE -> OFF)\n");
    
    while (true) {
        float speed = GetVelocity();
        currentSpeed = speed;
        
        if (speed > 10.0f) {
            if (speed > maxSpeed) maxSpeed = speed;
            wasMoving = true;
        } else {
            if (wasMoving && maxSpeed > 0) {
                lastSpeed = maxSpeed;
                maxSpeed = 0;
            }
            wasMoving = false;
        }
        
        UpdateOverlay();
        
        if (editMode > 0) {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        
        if (GetAsyncKeyState(VK_HOME) & 1) {
            static bool consoleVisible = true;
            consoleVisible = !consoleVisible;
            ShowWindow(GetConsoleWindow(), consoleVisible ? SW_SHOW : SW_HIDE);
        }
        
        if (GetAsyncKeyState(VK_DELETE) & 1) {
            static bool overlayVisible = true;
            overlayVisible = !overlayVisible;
            ShowWindow(overlayWnd, overlayVisible ? SW_SHOW : SW_HIDE);
        }
        
        if (GetAsyncKeyState(VK_END) & 1) {
            int newMode = (editMode + 1) % 3;
            SetEditMode(newMode);
            const char* modeNames[] = { "OFF", "MOVE", "RESIZE" };
            printf("Edit mode: %s\n", modeNames[editMode]);
        }
        
        MSG msg;
        while (PeekMessage(&msg, overlayWnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        Sleep(16);
    }
    
    RemoveMouseHooks();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        hModule = hMod;
        DisableThreadLibraryCalls(hMod);
        CreateThread(NULL, 0, MainThread, hMod, 0, NULL);
    }
    return TRUE;
}
