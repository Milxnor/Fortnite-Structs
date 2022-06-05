#include <Windows.h>
#include <fstream>
#include <iostream>

// #include "structs.h"
#include "xorstr.hpp"
// #include "finder.h"
#include "newstructs.h"

DWORD WINAPI CreateConsole(LPVOID)
{
    static auto Engine = FindObject("FortEngine_");

    std::cout << "Member count: " << GetMembers(Engine).size() << '\n';

    for (auto& Member : GetMembers(Engine))
	{
		std::cout << "Member: " << Member->GetName() << '\n';
	}
    // std::cout << "GameViewport: " << *Engine->Member<UObject*>(_("GameViewport")) << '\n';
}

DWORD WINAPI Input(LPVOID)
{
    while (1)
    {	
        Sleep(1000 / 30);
    }
}

DWORD WINAPI Main(LPVOID) // Example code
{
    AllocConsole();

    FILE* file;
    freopen_s(&file, _("CONOUT$"), _("w"), stdout);
	
    if (!Setup())
    {
        std::cout << _("Failed Setup!\n");
        return 0;
    }

    InitializeOffsets();

    std::cout << _("Fortnite Version: ") << FN_Version << '\n';

    CreateThread(0, 0, CreateConsole, 0, 0, 0);
    // CreateThread(0, 0, SpawnCheatManager, 0, 0, 0);
    // CreateThread(0, 0, Input, 0, 0, 0);

    std::cout << "Offset_Internal: " << Offsets::Offset_Internal << '\n';

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dllReason, LPVOID lpReserved)
{
    switch (dllReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}