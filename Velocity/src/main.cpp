#include <Windows.h>
#include <d3d9.h>
#include <cmath>
#include <cstdio>
#include <Psapi.h>
#include <vector>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "psapi.lib")

// ============ NETVARS ============
namespace netvars {
    uintptr_t m_iHealth = 0x100;
    uintptr_t m_iTeamNum = 0xF4;
    uintptr_t m_vecVelocity = 0x114;
}

// Глобальные
uintptr_t clientBase = 0;
uintptr_t clientSize = 0;
uintptr_t dwLocalPlayer = 0;

float currentSpeed = 0.0f;
float lastSpeed = 0.0f;
float maxSpeed = 0.0f;
bool wasMoving = false;
bool shouldExit = false;

HFONT hFont = NULL;
HMODULE hModule = NULL;
HWND gameWnd = NULL;
WNDPROC oWndProc = nullptr;

// ============ MEMORY ============
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

// ============ PATTERN SCANNING ============
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

// ============ GAME ============
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

// ============ OVERLAY WINDOW ============
HWND overlayWnd = NULL;
bool overlayVisible = true;

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, hFont);
        
        char text[64];
        sprintf_s(text, "%d (%d)", (int)currentSpeed, (int)lastSpeed);
        
        // Центр экрана
        SIZE textSize;
        GetTextExtentPoint32A(hdc, text, (int)strlen(text), &textSize);
        int x = (rc.right - textSize.cx) / 2;
        int y = rc.bottom / 2 + 100;
        
        // Тень
        SetTextColor(hdc, RGB(0, 80, 0));
        TextOutA(hdc, x + 2, y + 2, text, (int)strlen(text));
        
        // Основной текст
        SetTextColor(hdc, RGB(0, 255, 0));
        TextOutA(hdc, x, y, text, (int)strlen(text));
        
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        overlayWnd = NULL;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void CreateOverlayWindow() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = hModule;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = "VelocityOverlayInternal";
    RegisterClassExA(&wc);
    
    RECT gameRect;
    GetClientRect(gameWnd, &gameRect);
    
    POINT pt = { 0, 0 };
    ClientToScreen(gameWnd, &pt);
    
    overlayWnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "VelocityOverlayInternal",
        NULL,
        WS_POPUP,
        pt.x, pt.y,
        gameRect.right, gameRect.bottom,
        NULL, NULL, hModule, NULL
    );
    
    SetLayeredWindowAttributes(overlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    SetWindowDisplayAffinity(overlayWnd, WDA_EXCLUDEFROMCAPTURE);
    
    ShowWindow(overlayWnd, SW_SHOW);
}

void UpdateOverlayPosition() {
    if (!overlayWnd || !gameWnd) return;
    
    RECT gameRect;
    GetClientRect(gameWnd, &gameRect);
    
    POINT pt = { 0, 0 };
    ClientToScreen(gameWnd, &pt);
    
    SetWindowPos(overlayWnd, HWND_TOPMOST,
        pt.x, pt.y,
        gameRect.right, gameRect.bottom,
        SWP_NOACTIVATE);
}

// ============ MAIN THREAD ============
DWORD WINAPI MainThread(LPVOID lpParam) {
    // Ждём client.dll
    while (!(clientBase = GetModuleInfo(L"client.dll", &clientSize))) {
        Sleep(100);
    }
    Sleep(500);
    
    // Консоль
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    printf("=== Velocity Internal ===\n");
    printf("client.dll: 0x%IX\n", clientBase);
    
    // Pattern scan
    dwLocalPlayer = FindLocalPlayer();
    if (dwLocalPlayer) {
        printf("dwLocalPlayer: 0x%X\n", (unsigned)dwLocalPlayer);
    } else {
        printf("Pattern not found!\n");
    }
    
    // Ждём окно игры
    while (!(gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9"))) {
        gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive");
        if (gameWnd) break;
        Sleep(100);
    }
    printf("Game window: 0x%IX\n", (uintptr_t)gameWnd);
    
    // Шрифт
    hFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    
    // Создаём overlay
    CreateOverlayWindow();
    printf("Overlay created!\n");
    printf("\nHOME to unload.\n");
    
    // Главный цикл
    while (!shouldExit) {
        // Обновляем скорость
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
        
        // Обновляем overlay
        UpdateOverlayPosition();
        if (overlayWnd) {
            InvalidateRect(overlayWnd, NULL, TRUE);
            UpdateWindow(overlayWnd);
        }
        
        // HOME для выхода
        if (GetAsyncKeyState(VK_HOME) & 1) {
            shouldExit = true;
        }
        
        // Обработка сообщений overlay
        MSG msg;
        while (PeekMessage(&msg, overlayWnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        Sleep(16);
    }
    
    // Cleanup
    printf("Unloading...\n");
    
    if (overlayWnd) DestroyWindow(overlayWnd);
    if (hFont) DeleteObject(hFont);
    
    Sleep(100);
    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
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
