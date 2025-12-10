#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cmath>
#include <thread>
#include <chrono>
#include "offsets.hpp"

// Глобальные переменные
HANDLE hProcess = nullptr;
uintptr_t clientBase = 0;
uintptr_t engineBase = 0;

// Получение базового адреса модуля
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

// Получение PID процесса
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


// Чтение памяти
template<typename T>
T ReadMemory(uintptr_t address) {
    T value = T();
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
    return value;
}

// Получение локального игрока
uintptr_t GetLocalPlayer() {
    return ReadMemory<uintptr_t>(clientBase + hazedumper::signatures::dwLocalPlayer);
}

// Получение скорости
float GetVelocity() {
    uintptr_t localPlayer = GetLocalPlayer();
    if (!localPlayer) return 0.0f;
    
    float velX = ReadMemory<float>(localPlayer + hazedumper::netvars::m_vecVelocity);
    float velY = ReadMemory<float>(localPlayer + hazedumper::netvars::m_vecVelocity + 4);
    
    return std::sqrt(velX * velX + velY * velY);
}

int main() {
    SetConsoleTitleW(L"CS:GO Velocity");
    std::wcout << L"Ожидание запуска CS:GO..." << std::endl;
    
    // Ждём запуска игры
    DWORD pid = 0;
    while (!(pid = GetProcessId(L"csgo.exe"))) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Открываем процесс
    hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        std::wcout << L"Не удалось открыть процесс!" << std::endl;
        return 1;
    }
    
    // Получаем базовые адреса модулей
    clientBase = GetModuleBaseAddress(pid, L"client.dll");
    engineBase = GetModuleBaseAddress(pid, L"engine.dll");
    
    if (!clientBase) {
        std::wcout << L"Не удалось найти client.dll!" << std::endl;
        CloseHandle(hProcess);
        return 1;
    }
    
    std::wcout << L"CS:GO найден!" << std::endl;
    std::wcout << L"client.dll: 0x" << std::hex << clientBase << std::dec << std::endl;
    std::wcout << L"\nНажми любую клавишу для выхода...\n" << std::endl;
    
    float prevSpeed = 0.0f;
    float maxSpeed = 0.0f;
    bool wasMoving = false;
    
    // Основной цикл
    while (!GetAsyncKeyState(VK_ESCAPE)) {
        float speed = GetVelocity();
        
        bool isMoving = speed > 10.0f;
        if (isMoving) {
            if (speed > maxSpeed) maxSpeed = speed;
            wasMoving = true;
        } else {
            if (wasMoving && maxSpeed > 0) {
                prevSpeed = maxSpeed;
                maxSpeed = 0;
            }
            wasMoving = false;
        }
        
        // Очистка строки и вывод скорости
        std::wcout << L"\r                              \r";
        std::wcout << L"Скорость: " << (int)speed << L" (" << (int)prevSpeed << L")" << std::flush;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    CloseHandle(hProcess);
    return 0;
}
