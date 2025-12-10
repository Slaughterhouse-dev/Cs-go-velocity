#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>

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

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        printf("Error: Cannot open process. Run as admin!\n");
        return false;
    }
    
    void* allocMem = VirtualAllocEx(hProcess, nullptr, strlen(dllPath) + 1, 
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        printf("Error: VirtualAllocEx failed\n");
        CloseHandle(hProcess);
        return false;
    }
    
    WriteProcessMemory(hProcess, allocMem, dllPath, strlen(dllPath) + 1, nullptr);
    
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        allocMem, 0, nullptr);
    
    if (!hThread) {
        printf("Error: CreateRemoteThread failed\n");
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    WaitForSingleObject(hThread, INFINITE);
    
    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return true;
}

int main(int argc, char* argv[]) {
    printf("=== Velocity Injector ===\n");
    printf("Waiting for CS:GO...\n");
    
    DWORD pid = 0;
    while (!(pid = GetProcessId(L"csgo.exe"))) {
        Sleep(1000);
    }
    
    printf("CS:GO found! PID: %lu\n", pid);
    
    // Путь к DLL
    char dllPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, dllPath);
    strcat_s(dllPath, "\\Velocity.dll");
    
    printf("Injecting: %s\n", dllPath);
    
    if (InjectDLL(pid, dllPath)) {
        printf("Success! Press DELETE in game to unload.\n");
    } else {
        printf("Injection failed!\n");
    }
    
    system("pause");
    return 0;
}
