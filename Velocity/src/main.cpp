#include <cstdio>
#include <Windows.h>
#include <TlHelp32.h>
#include <cmath>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdi32.lib")

// Оффсеты для CS:GO
namespace offsets {
    constexpr uintptr_t dwLocalPlayer = 0xDEB99C;
    constexpr uintptr_t m_vecVelocity = 0x114;
}

// Глобальные переменные
HANDLE hProcess = nullptr;
uintptr_t clientBase = 0;
HWND overlayWnd = nullptr;
HWND gameWnd = nullptr;
float currentSpeed = 0.0f;
float lastSpeed = 0.0f;
bool isRunning = true;
HFONT hFont = nullptr;

uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName) {
    uintptr_t baseAddress = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32FirstW(hSnap, &modEntry)) {
            do {
                if (_wcsicmp(modEntry.szModule, moduleName) == 0) {
                    baseAddress = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return baseAddress;
}

DWORD GetProcessId(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
    return pid;
}

template<typename T>
T ReadMemory(uintptr_t address) {
    T value = T();
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
    return value;
}

uintptr_t GetLocalPlayer() {
    // Пробуем прямое чтение
    uintptr_t player = ReadMemory<uintptr_t>(clientBase + offsets::dwLocalPlayer);
    if (player) return player;
    
    // Пробуем как указатель на указатель
    uintptr_t ptr = ReadMemory<uintptr_t>(clientBase + offsets::dwLocalPlayer);
    if (ptr) {
        player = ReadMemory<uintptr_t>(ptr);
    }
    return player;
}

float GetVelocity() {
    uintptr_t localPlayer = GetLocalPlayer();
    if (!localPlayer) {
        printf("\rLocalPlayer: NULL                    ");
        return 0.0f;
    }
    float velX = ReadMemory<float>(localPlayer + offsets::m_vecVelocity);
    float velY = ReadMemory<float>(localPlayer + offsets::m_vecVelocity + 4);
    float speed = sqrtf(velX * velX + velY * velY);
    printf("\rLP: 0x%IX | Vel: %.1f, %.1f | Speed: %.0f   ", localPlayer, velX, velY, speed);
    return speed;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // Прозрачный фон
            SetBkMode(hdc, TRANSPARENT);
            
            // Зелёный текст как на скриншоте
            SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(0, 255, 0));
            
            // Формат: "247 (240)"
            char text[64];
            sprintf_s(text, "%d (%d)", (int)currentSpeed, (int)lastSpeed);
            
            // Центр экрана, чуть ниже середины
            int x = rect.right / 2;
            int y = rect.bottom / 2 + 100;
            
            // Тень для читаемости
            SetTextColor(hdc, RGB(0, 50, 0));
            TextOutA(hdc, x - 48, y + 2, text, (int)strlen(text));
            
            // Основной текст
            SetTextColor(hdc, RGB(0, 255, 0));
            TextOutA(hdc, x - 50, y, text, (int)strlen(text));
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            isRunning = false;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void UpdateOverlayPosition() {
    if (!gameWnd || !overlayWnd) return;
    
    RECT gameRect;
    GetWindowRect(gameWnd, &gameRect);
    
    int width = gameRect.right - gameRect.left;
    int height = gameRect.bottom - gameRect.top;
    
    SetWindowPos(overlayWnd, HWND_TOPMOST, 
        gameRect.left, gameRect.top, width, height,
        SWP_NOACTIVATE);
}

HWND CreateOverlayWindow() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "VelocityOverlay";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        "VelocityOverlay", "",
        WS_POPUP,
        0, 0, 1920, 1080,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );

    // Чёрный = прозрачный
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

DWORD WINAPI UpdateThread(LPVOID) {
    float maxSpeed = 0.0f;
    bool wasMoving = false;
    
    while (isRunning) {
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
        
        // Обновляем позицию и перерисовываем
        UpdateOverlayPosition();
        InvalidateRect(overlayWnd, nullptr, TRUE);
        
        Sleep(16);
    }
    return 0;
}

int main() {
    printf("=== CS:GO Velocity ===\n");
    printf("Waiting for CS:GO...\n");
    
    DWORD pid = 0;
    while (!(pid = GetProcessId(L"csgo.exe"))) {
        Sleep(1000);
    }
    
    printf("CS:GO found! PID: %lu\n", pid);
    
    hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        printf("Error: Run as admin!\n");
        system("pause");
        return 1;
    }
    
    clientBase = GetModuleBaseAddress(pid, L"client.dll");
    if (!clientBase) {
        printf("Error: client.dll not found!\n");
        CloseHandle(hProcess);
        system("pause");
        return 1;
    }
    
    // Ищем окно игры
    gameWnd = FindWindowA(nullptr, "Counter-Strike: Global Offensive");
    if (!gameWnd) {
        gameWnd = FindWindowA(nullptr, "Counter-Strike: Global Offensive - Direct3D 9");
    }
    
    printf("client.dll: 0x%IX\n", clientBase);
    printf("Press ESC to exit\n");
    
    // Шрифт как на скриншоте
    hFont = CreateFontA(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Arial");
    
    overlayWnd = CreateOverlayWindow();
    CreateThread(nullptr, 0, UpdateThread, nullptr, 0, nullptr);
    
    MSG msg;
    while (isRunning) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;
        Sleep(1);
    }
    
    DeleteObject(hFont);
    CloseHandle(hProcess);
    return 0;
}
