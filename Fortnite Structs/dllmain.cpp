#include <Windows.h>
#include <fstream>

#include "structs.h"
#include "xorstr.hpp"
#include "finder.h"

DWORD WINAPI CreateConsole(LPVOID)
{
    // auto t = new Timer;

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

    // delete t;

    std::cout << _("Console created!\n");

    return 0;
}

DWORD WINAPI SpawnCheatManager(LPVOID)
{
    Finder Engine("FortEngine_");
    auto PC = Engine["GameInstance"].ArrayAt<UObject*>(_("LocalPlayers"), 0)["PlayerController"].m_Object;
    UObject* CheatManagerClass = FindObject(_("Class /Script/FortniteGame.FortCheatManager"));

    auto CheatManager = PC->Member<UObject*>(_("CheatManager"));
    std::vector<Var> Args = { CheatManagerClass, PC };

    Finder gscFinder("GameplayStatics /Script/Engine.Default__GameplayStatics");
    auto ret = gscFinder.Call("SpawnObject", Args, sizeof(UObject*));
    std::cout << "Ret: " << ret << '\n';
    // *CheatManager = (UObject*)ret;
}

DWORD WINAPI Input(LPVOID)
{
    while (1)
    {
        if (GetAsyncKeyState(VK_F1) & 1)
        {
            CreateThread(0, 0, SpawnCheatManager, 0, 0, 0);
            std::cout << "Creating CheatManager!\n";
        }
		
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

    std::cout << _("Fortnite Version: ") << FN_Version << '\n';

    CreateThread(0, 0, CreateConsole, 0, 0, 0);
    CreateThread(0, 0, SpawnCheatManager, 0, 0, 0);
    // CreateThread(0, 0, Input, 0, 0, 0);

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