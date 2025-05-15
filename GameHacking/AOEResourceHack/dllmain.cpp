// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include<TlHelp32.h>
#include <iostream>
#include <tchar.h> // _tcscmp
#include <vector>

DWORD GetModuleBaseAddress(TCHAR* lpszModuleName, DWORD pID) {
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID); // make snapshot of all modules within process
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32)) //store first Module in ModuleEntry32
    {
        do {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
            {
                dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32


    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
}

HANDLE GetProcessHandle(const wchar_t* window) {
    HWND hGameWindow = FindWindow(NULL, L"Age of Empires Expansion");
    if (hGameWindow == NULL) {
        std::cout << "Start the game!" << std::endl;
        return 0;
    }
    DWORD pID = NULL; // ID of our Game
    GetWindowThreadProcessId(hGameWindow, &pID);
    HANDLE processHandle = NULL;
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (processHandle == INVALID_HANDLE_VALUE || processHandle == NULL) { // error handling
        std::cout << "Failed to open process" << std::endl;
        return 0;
    }
    return processHandle;
}

void AddFood(HANDLE processHandle) {
    DWORD pID = GetProcessId(processHandle);
    TCHAR gameName[13];
    wcscpy_s(gameName, 13, L"EMPIRESX.EXE");

    DWORD gameBaseAddress = GetModuleBaseAddress(gameName, pID);


    DWORD offsetGameToBaseAdress = 0x00188144;
    std::vector<DWORD> pointsOffsets{ 0x54C, 0x750, 0x100, 0x50, 0x0 };

    DWORD baseAddress = NULL;

    //Get value at gamebase+offset -> store it in baseAddress
    ReadProcessMemory(processHandle, (LPVOID)(gameBaseAddress + offsetGameToBaseAdress), &baseAddress, sizeof(baseAddress), NULL);

    DWORD pointsAddress = baseAddress; //the Adress we need -> change now while going through offsets
    for (int i = 0; i < pointsOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
    {
        if (i != pointsOffsets.size() - 1)
            ReadProcessMemory(processHandle, (LPVOID)(pointsAddress + pointsOffsets.at(i)), &pointsAddress, sizeof(pointsAddress), NULL);
        else
            pointsAddress += pointsOffsets.at(i);
    }
    float currentPoint = 0;
    ReadProcessMemory(processHandle, (LPVOID)(pointsAddress), &currentPoint, sizeof(currentPoint), NULL);
    float newPoints = currentPoint + 100;
    WriteProcessMemory(processHandle, (LPVOID)(pointsAddress), &newPoints, 4, 0);
}

DWORD WINAPI MainThread(LPVOID param) {
    const wchar_t* window = L"Age of Empires Expansion";
    HANDLE gameProcessHandle = GetProcessHandle(window);
    while (true) {
        if (GetAsyncKeyState(VK_F6) & 0x80000) {
            //MessageBoxA(NULL, "F6 Pressed!", "F6 Pressed!", MB_OK);
            AddFood(gameProcessHandle);
        }
        Sleep(100);
    }
    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MessageBoxA(NULL, "DLL Injected!", "DLL Injected!", MB_OK);
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

