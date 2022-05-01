#include <Windows.h>
#include <fstream>

#include "structs.h"
#include "xorstr.hpp"

DWORD WINAPI CreateConsole(LPVOID)
{
    auto t = new Timer;

    auto Engine = FindObject(_("FortEngine_"));

    while (!Engine) // we have to take in account for FEngineLoop
    {
        Engine = FindObject(_("FortEngine_"));
        Sleep(1000 / 30);
    }

    static auto ConsoleClass = FindObject(_("Class /Script/Engine.Console"));
    static auto GameViewport = Engine->Member<UObject*>(_("GameViewport"));
    UObject** ViewportConsole = (*GameViewport)->Member<UObject*>(_("ViewportConsole"));

    struct {
        UObject* ObjectClass;
        UObject* Outer;
        UObject* ReturnValue;
    } params{};

    params.ObjectClass = ConsoleClass;
    params.Outer = *GameViewport;

    static auto GSC = FindObject(_("GameplayStatics /Script/Engine.Default__GameplayStatics"));
    static auto fn = GSC->Function(_("SpawnObject"));
	// static auto fn = FindObject(_("Function /Script/Engine.GameplayStatics.SpawnObject"));

    GSC->ProcessEvent(fn, &params);

    *ViewportConsole = params.ReturnValue;

    delete t;

    std::cout << _("Console created!\n");

    return 0;
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

    std::cout << _("Fortnite Version: ") << FN_Version << '\n';

    auto Engine = FindObject(_("FortEngine_"));

    std::cout << "World:" << (*(*FindObject(_("FortEngine_"))->Member<UObject*>(_("GameViewport")))->Member<UObject*>(_("World")))->GetFullName() << '\n';

    CreateThread(0, 0, CreateConsole, 0, 0, 0);

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