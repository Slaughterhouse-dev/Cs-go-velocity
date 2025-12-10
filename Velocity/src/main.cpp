#include <Windows.h>
#include <TlHelp32.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

// ============ NETVARS ============
namespace netvars {
    uintptr_t m_iHealth = 0x100;
    uintptr_t m_iTeamNum = 0xF4;
    uintptr_t m_vecVelocity = 0x114;
}

// Глобальные
HANDLE hProcess = NULL;
HWND gameWnd = NULL;
HWND overlayWnd = NULL;
uintptr_t clientBase = 0;
uintptr_t clientSize = 0;
uintptr_t dwLocalPlayer = 0;

float currentSpeed = 0.0f;
float lastSpeed = 0.0f;
float maxSpeed = 0.0f;
bool wasMoving = false;
bool isRunning = true;

HFONT hFont = NULL;

// ============ MEMORY ============
DWORD GetProcessId(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    return pid;
}

uintptr_t GetModuleBase(DWORD pid, const wchar_t* moduleName, uintptr_t* size) {
    uintptr_t base = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                if (_wcsicmp(me.szModule, moduleName) == 0) {
                    base = (uintptr_t)me.modBaseAddr;
                    if (size) *size = me.modBaseSize;
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
    }
    return base;
}

template<typename T>
T Read(uintptr_t address) {
    T value = T();
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
    return value;
}

// ============ PATTERN SCANNING ============
std::vector<BYTE> ParsePattern(const char* pattern, std::vector<bool>& mask) {
    std::vector<BYTE> bytes;
    mask.clear();
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
    return bytes;
}

uintptr_t PatternScan(uintptr_t base, uintptr_t size, const char* pattern) {
    std::vector<bool> mask;
    std::vector<BYTE> patternBytes = ParsePattern(pattern, mask);
    const SIZE_T chunkSize = 0x100000;
    BYTE* buffer = new BYTE[chunkSize + patternBytes.size()];
    
    for (uintptr_t offset = 0; offset < size; offset += chunkSize) {
        SIZE_T toRead = min(chunkSize + patternBytes.size(), size - offset);
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)(base + offset), buffer, toRead, &bytesRead))
            continue;
        for (SIZE_T i = 0; i < bytesRead - patternBytes.size(); i++) {
            bool found = true;
            for (SIZE_T j = 0; j < patternBytes.size(); j++) {
                if (mask[j] && buffer[i + j] != patternBytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                delete[] buffer;
                return base + offset + i;
            }
        }
    }
    delete[] buffer;
    return 0;
}

uintptr_t FindLocalPlayer() {
    const char* pattern = "8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF";
    uintptr_t addr = PatternScan(clientBase, clientSize, pattern);
    if (!addr) return 0;
    uintptr_t ptr = Read<uintptr_t>(addr + 3);
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

// ============ OVERLAY ============
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, MSG* pMsg, LRESULT* pResult) {
    return DefWindowProcA(hwnd, msg, (WPARAM)pMsg, (LPARAM)pResult);
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Прозрачный фон
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        // Рисуем текст
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, hFont);
        
        char text[64];
        sprintf_s(text, "%d (%d)", (int)currentSpeed, (int)lastSpeed);
        
        int x = rc.right / 2;
        int y = rc.bottom / 2 + 100;
        
        // Тень
        SetTextColor(hdc, RGB(0, 80, 0));
        TextOutA(hdc, x + 2, y + 2, text, (int)strlen(text));
        
        // Основной текст - зелёный
        SetTextColor(hdc, RGB(0, 255, 0));
        TextOutA(hdc, x, y, text, (int)strlen(text));
        
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

void CreateOverlay() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = "VelocityOverlay";
    RegisterClassExA(&wc);
    
    RECT gameRect;
    GetWindowRect(gameWnd, &gameRect);
    
    overlayWnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "VelocityOverlay",
        "Velocity",
        WS_POPUP,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left,
        gameRect.bottom - gameRect.top,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    // Прозрачность - чёрный цвет будет прозрачным
    SetLayeredWindowAttributes(overlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // Исключить из захвата экрана (OBS, записи и т.д.)
    SetWindowDisplayAffinity(overlayWnd, WDA_EXCLUDEFROMCAPTURE);
    
    // Шрифт
    hFont = CreateFontA(
        48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial"
    );
    
    ShowWindow(overlayWnd, SW_SHOW);
    UpdateWindow(overlayWnd);
}

void UpdateOverlay() {
    if (!gameWnd || !overlayWnd) return;
    
    RECT gameRect;
    GetWindowRect(gameWnd, &gameRect);
    
    SetWindowPos(overlayWnd, HWND_TOPMOST,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left,
        gameRect.bottom - gameRect.top,
        SWP_NOACTIVATE);
    
    InvalidateRect(overlayWnd, NULL, TRUE);
}

// ============ MAIN ============
DWORD WINAPI UpdateThread(LPVOID) {
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
        
        // Проверяем окно игры
        gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9");
        if (!gameWnd) gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive");
        
        if (gameWnd && overlayWnd) {
            UpdateOverlay();
        }
        
        // HOME для выхода
        if (GetAsyncKeyState(VK_HOME) & 1) {
            isRunning = false;
        }
        
        Sleep(16);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Консоль для отладки
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    printf("=== Velocity Overlay ===\n\n");
    
    // Ждём CS:GO
    printf("Waiting for csgo.exe...\n");
    DWORD pid = 0;
    while (!(pid = GetProcessId(L"csgo.exe"))) {
        Sleep(1000);
    }
    printf("Found csgo.exe (PID: %d)\n", pid);
    
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        printf("Failed! Run as Administrator.\n");
        Sleep(3000);
        return 1;
    }
    
    // Ждём client.dll
    printf("Waiting for client.dll...\n");
    while (!(clientBase = GetModuleBase(pid, L"client.dll", &clientSize))) {
        Sleep(500);
    }
    printf("client.dll: 0x%IX\n", clientBase);
    
    // Pattern scan
    printf("Pattern scanning...\n");
    dwLocalPlayer = FindLocalPlayer();
    if (dwLocalPlayer) {
        printf("dwLocalPlayer: 0x%X\n", (unsigned)dwLocalPlayer);
    } else {
        printf("Pattern not found!\n");
    }
    
    // Ждём окно игры
    printf("Waiting for game window...\n");
    while (!(gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9"))) {
        gameWnd = FindWindowA(NULL, "Counter-Strike: Global Offensive");
        if (gameWnd) break;
        Sleep(500);
    }
    printf("Game window found!\n");
    
    // Создаём overlay
    CreateOverlay();
    printf("\nOverlay created. HOME to exit.\n");
    
    // Поток обновления
    CreateThread(NULL, 0, UpdateThread, NULL, 0, NULL);
    
    // Message loop
    MSG msg;
    while (isRunning && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if (hFont) DeleteObject(hFont);
    if (overlayWnd) DestroyWindow(overlayWnd);
    CloseHandle(hProcess);
    if (f) fclose(f);
    FreeConsole();
    
    return 0;
}
