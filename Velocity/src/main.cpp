#include <Windows.h>
#include <cmath>
#include <cstdio>

// Оффсеты CS:GO Legacy (hazedumper sept 2023)
namespace offsets {
    constexpr uintptr_t dwLocalPlayer = 0xDEB99C;
    constexpr uintptr_t m_vecVelocity = 0x114;
    constexpr uintptr_t m_iHealth = 0x100;
}

// Глобальные переменные
uintptr_t clientBase = 0;
float currentSpeed = 0.0f;
float lastSpeed = 0.0f;
float maxSpeed = 0.0f;
bool wasMoving = false;
bool shouldExit = false;

// Получение модуля
uintptr_t GetModuleBaseAddress(const wchar_t* moduleName) {
    return (uintptr_t)GetModuleHandleW(moduleName);
}

// Чтение памяти (internal - напрямую)
template<typename T>
T Read(uintptr_t address) {
    if (address == 0) return T();
    __try {
        return *(T*)address;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T();
    }
}

// Получение локального игрока
uintptr_t GetLocalPlayer() {
    return Read<uintptr_t>(clientBase + offsets::dwLocalPlayer);
}

// Получение скорости
float GetVelocity() {
    uintptr_t localPlayer = GetLocalPlayer();
    if (!localPlayer) return 0.0f;
    
    float velX = Read<float>(localPlayer + offsets::m_vecVelocity);
    float velY = Read<float>(localPlayer + offsets::m_vecVelocity + 4);
    return sqrtf(velX * velX + velY * velY);
}

// Главный поток DLL
DWORD WINAPI MainThread(LPVOID lpParam) {
    // Ждём загрузки модулей
    while (!(clientBase = GetModuleBaseAddress(L"client.dll"))) {
        Sleep(100);
    }
    
    // Консоль для отладки
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    printf("=== Velocity Internal ===\n");
    printf("client.dll: 0x%IX\n", clientBase);
    printf("dwLocalPlayer offset: 0x%X\n", offsets::dwLocalPlayer);
    printf("\nPress DELETE to unload\n");
    printf("Press INSERT to toggle console\n\n");
    
    bool consoleVisible = true;
    HWND consoleWnd = GetConsoleWindow();
    
    while (!shouldExit) {
        // Toggle console
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            consoleVisible = !consoleVisible;
            ShowWindow(consoleWnd, consoleVisible ? SW_SHOW : SW_HIDE);
        }
        
        // Exit
        if (GetAsyncKeyState(VK_DELETE) & 1) {
            shouldExit = true;
            break;
        }
        
        uintptr_t localPlayer = GetLocalPlayer();
        float speed = GetVelocity();
        currentSpeed = speed;
        
        // Логика последней скорости
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
        
        if (localPlayer) {
            int health = Read<int>(localPlayer + offsets::m_iHealth);
            printf("\rLP: 0x%IX | HP: %d | Speed: %d (%d)      ", 
                localPlayer, health, (int)currentSpeed, (int)lastSpeed);
        } else {
            printf("\rLocalPlayer: NULL - join a map!          ");
        }
        
        Sleep(16);
    }
    
    printf("\n\nUnloading...\n");
    Sleep(500);
    
    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
