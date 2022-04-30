#include <Windows.h>
#include <fstream>

#include "structs.h"
#include "xorstr.hpp"

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

    auto Engine = FindObject(_("FortEngine_"));

    if (Engine)
    {
        std::cout << "Engine: " << Engine->GetFullName() << '\n';
        // std::cout << "GameInstance: " << (*Engine->Member<UObject*>(_("GameInstance")))->GetFullName() << '\n';;
    }
	
    else
	{
		std::cout << _("Failed to find Engine!\n");
		return 0;
	}

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