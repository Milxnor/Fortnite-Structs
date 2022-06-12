#include <Windows.h>
#include <cstdint>
#include <string>
#include <locale>
#include <format>
#include "other.h"
#include <regex>

static inline void (*ToStringO)(struct FName*, class FString&);
static inline void* (*ProcessEventO)(void*, void*, void*);

std::string FN_Version;
static int EngineVersion = 0;

static struct FChunkedFixedUObjectArray* ObjObjects;
static struct FFixedUObjectArray* OldObjects;

namespace FMemory
{
	void (*Free)(void* Original);
	void (*Realloc)(void* Original, SIZE_T Count, uint32_t Alignment /* = DEFAULT_ALIGNMENT */);
}

namespace Offsets
{
	int Super = 0x30;
	int ChildProperties = 0x50;
	int Children = 0x38;
	int Offset_Internal = 0x44;
}

void InitializeOffsets()
{
	if (EngineVersion >= 422)
	{
		Offsets::Children = 0x48;
		Offsets::Super = 0x40;
	}

	if (EngineVersion >= 425 && EngineVersion < 500)
	{
		Offsets::Offset_Internal = 0x4C;
	}
}

template <class ElementType>
class TArray // https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Source/Runtime/Core/Public/Containers/Array.h#L305
{
	friend class FString;
protected:
	ElementType* Data = nullptr;
	int32_t ArrayNum = 0;
	int32_t ArrayMax = 0;
public:
	void Free()
	{
		if (FMemory::Free)
			FMemory::Free(Data);

		Data = nullptr;

		ArrayNum = 0;
		ArrayMax = 0;
	}

	__forceinline ElementType& operator[](int Index) const { return Data[Index]; }

	__forceinline ElementType& At(int Index) const { return Data[Index]; }

	__forceinline int32_t Slack() const
	{
		return ArrayMax - ArrayNum;
	}

	__forceinline void Reserve(int Number)
	{
		if (!FMemory::Realloc)
		{
			MessageBoxA(0, _("How are you expecting to reserve with no Realloc?"), _("Fortnite"), MB_ICONERROR);
			return;
		}

		if (Number > ArrayMax)
			Data = Slack() >= ArrayNum ? Data : (ElementType*)FMemory::Realloc(Data, (ArrayMax = ArrayNum + ArrayNum) * sizeof(ElementType), 0); // thanks fischsalat for this line of code.
	}

	__forceinline void Add(const ElementType& New)
	{
		Reserve(1);
		Data[ArrayNum] = New;
		++ArrayNum;
	};

	__forceinline auto GetData() const { return Data; }
};

class FString // https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Source/Runtime/Core/Public/Containers/UnrealString.h#L59
{
public:
	TArray<TCHAR> Data;

	void Set(const wchar_t* NewStr) // by fischsalat
	{
		if (!NewStr || std::wcslen(NewStr) == 0) return;

		Data.ArrayMax = Data.ArrayNum = *NewStr ? (int)std::wcslen(NewStr) + 1 : 0;

		if (Data.ArrayNum)
			Data.Data = const_cast<wchar_t*>(NewStr);
	}

	std::string ToString() const
	{
		auto length = std::wcslen(Data.Data);
		std::string str(length, '\0');
		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data.Data, Data.Data + length, '?', &str[0]);

		return str;
	}

	void FreeString()
	{
		Data.Free();
	}

	~FString()
	{
		// FreeString();
	}
};

struct FName // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/Core/Public/UObject/NameTypes.h#L403
{
	uint32_t ComparisonIndex;
	uint32_t Number;

	__forceinline std::string ToString()
	{
		FString temp;

		ToStringO(this, temp);

		auto Str = temp.ToString();

		temp.FreeString();

		return Str;
	}
};

struct Property
{
	auto GetOffset()
	{
		return *(int*)(this + Offsets::Offset_Internal);
	}
};

struct UObject // https://github.com/EpicGames/UnrealEngine/blob/c3caf7b6bf12ae4c8e09b606f10a09776b4d1f38/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBase.h#L20
{
	void** VFTable;
	int32_t ObjectFlags;
	int32_t InternalIndex;
	UObject* ClassPrivate; // Keep it an object because the we will have to cast it to the correct type depending on the version.
	FName NamePrivate;
	UObject* OuterPrivate;

	__forceinline std::string GetName() { return NamePrivate.ToString(); }

	__forceinline std::string GetFullName()
	{
		std::string temp;

		for (auto outer = OuterPrivate; outer; outer = outer->OuterPrivate)
			temp = std::format("{}.{}", outer->GetName(), temp);

		return std::format("{} {}{}", ClassPrivate->GetName(), temp, this->GetName());
	}

	template <typename MemberType>
	__forceinline MemberType* Member(const std::string& MemberName);
};

struct UField : UObject
{
	UField* Next;

};

struct FField
{
	void** VFT;
	void* ClassPrivate;
	void* Owner;
	void* pad;
	FField* Next;
	FName Name;
	EObjectFlags FlagsPrivate;

	std::string GetName()
	{
		return Name.ToString();
	}
};

struct UStruct
{
	__forceinline auto GetChildren()
	{
		return *(UField**)(this + Offsets::Children);
	}

	__forceinline FField* GetChildProperties()
	{
		if (EngineVersion < 425)
			return nullptr;
		
		return *(FField**)(this + Offsets::ChildProperties);
	}
	
	__forceinline auto GetSuper()
	{
		return *(UStruct**)(this + Offsets::Super);
	}
};

auto GetMembers(UObject* Object, bool bOnlyMembers = false, bool bOnlyFunctions = false, std::vector<FField*>* OutProperties = nullptr)
{
	std::vector<UObject*> Members;

	if (Object)
	{
		for (auto Class = (UStruct*)Object->ClassPrivate; Class; Class = (UStruct*)Class->GetSuper())
		{
			auto Child = Class->GetChildren();
			
			if (OutProperties)
			{
				auto Property = Class->GetChildProperties();

				if (Property && (EngineVersion >= 425 && (bOnlyMembers && !bOnlyFunctions) || (!bOnlyMembers && !bOnlyFunctions)))
				{
					auto Next = Property->Next;

					if (Next)
					{
						while (Property)
						{
							// Members.push_back((UObject*)Property);
							OutProperties->push_back(Property);
							
							Property = Property->Next;
						}
					}
				}
			}

			if (Child && (EngineVersion >= 425 && (bOnlyMembers && !bOnlyFunctions) || (!bOnlyMembers && !bOnlyFunctions)))
			{
				auto Next = Child->Next;

				if (Next)
				{
					while (Child)
					{
						Members.push_back(Child);

						Child = Child->Next;
					}
				}
			}
		}
	}

	return Members;
}

struct FUObjectItem // https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h#L26
{
	UObject* Object;
	int32_t Flags;
	int32_t ClusterRootIndex;
	int32_t SerialNumber;
	// int pad_01;
};

struct FFixedUObjectArray
{
	FUObjectItem* Objects;
	int32_t MaxElements;
	int32_t NumElements;

	__forceinline const int32_t Num() const { return NumElements; }

	__forceinline const int32_t Capacity() const { return MaxElements; }

	__forceinline bool IsValidIndex(int32_t Index) const { return Index < Num() && Index >= 0; }

	__forceinline UObject* GetObjectById(int32_t Index) const
	{
		return Objects[Index].Object;
	}
};

struct FChunkedFixedUObjectArray // https://github.com/EpicGames/UnrealEngine/blob/7acbae1c8d1736bb5a0da4f6ed21ccb237bc8851/Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h#L321
{
	enum
	{
		NumElementsPerChunk = 64 * 1024,
	};

	FUObjectItem** Objects;
	FUObjectItem* PreAllocatedObjects;
	int32_t MaxElements;
	int32_t NumElements;
	int32_t MaxChunks;
	int32_t NumChunks;

	__forceinline const int32_t Num() const { return NumElements; }

	__forceinline const int32_t Capacity() const { return MaxElements; }

	__forceinline UObject* GetObjectById(int32_t Index) const
	{
		if (Index > NumElements || Index < 0) return nullptr;

		const int32_t ChunkIndex = Index / NumElementsPerChunk;
		const int32_t WithinChunkIndex = Index % NumElementsPerChunk;

		if (ChunkIndex > NumChunks) return nullptr;
		FUObjectItem* Chunk = Objects[ChunkIndex];
		if (!Chunk) return nullptr;

		auto obj = (Chunk + WithinChunkIndex)->Object;

		return obj;
	}
};

FString(*GetEngineVersion)();

// TODO: There is this 1.9 function, 48 8D 05 D9 51 22 03. It has the CL and stuff. We may be able to determine the version using the CL.
// There is also a string for the engine version and fortnite version, I think it's for every version its like "engineversion=". I will look into it when I find time.

bool Setup(/* void* ProcessEventHookAddr */) // TODO: Remake
{
	uint64_t ToStringAddr = 0;
	uint64_t ProcessEventAddr = 0;
	uint64_t ObjectsAddr = 0;
	uint64_t FreeMemoryAddr = 0;
	bool bOldObjects = false;

	GetEngineVersion = decltype(GetEngineVersion)(FindPattern(_("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3")));

	std::string FullVersion;
	FString toFree;

	if (!GetEngineVersion)
	{
		auto VerStr = FindPattern(_("2B 2B 46 6F 72 74 6E 69 74 65 2B 52 65 6C 65 61 73 65 2D ? ? ? ?"));

		if (!VerStr)
		{
			MessageBoxA(0, _("Failed to find fortnite version!"), _("Fortnite"), MB_ICONERROR);
			return false;
		}

		FullVersion = decltype(FullVersion.c_str())(VerStr);
		EngineVersion = 500;
	}

	else
	{
		toFree = GetEngineVersion();
		FullVersion = toFree.ToString();
	}

	std::string FNVer = FullVersion;
	std::string EngineVer = FullVersion;

	if (!FullVersion.contains(_("Live")) && !FullVersion.contains(_("Next")) && !FullVersion.contains(_("Cert")))
	{
		if (GetEngineVersion)
		{
			FNVer.erase(0, FNVer.find_last_of(_("-"), FNVer.length() - 1) + 1);
			EngineVer.erase(EngineVer.find_first_of(_("-"), FNVer.length() - 1), 40);

			if (EngineVer.find_first_of(".") != EngineVer.find_last_of(".")) // this is for 4.21.0 and itll remove the .0
				EngineVer.erase(EngineVer.find_last_of(_(".")), 2);

			EngineVersion = std::stod(EngineVer) * 100;
		}

		else
		{
			const std::regex base_regex(_("-([0-9.]*)-"));
			std::cmatch base_match;

			std::regex_search(FullVersion.c_str(), base_match, base_regex);

			FNVer = base_match[1];
		}

		FN_Version = FNVer;

		auto FnVerDouble = std::stod(FN_Version);

		if (FnVerDouble >= 16.00 && FnVerDouble < 18.40)
			EngineVersion = 427; // 4.26.1;
	}

	else
	{
		EngineVersion = 419;
		FN_Version = _("2.69");
	}

	if (EngineVersion >= 416 && EngineVersion <= 420)
	{
		ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76 30"), false, 7, true);

		if (!ObjectsAddr)
			ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8D 14 C8 EB 03 49 8B D6 8B 42 08 C1 E8 1D A8 01 0F 85 ? ? ? ? F7 86 ? ? ? ? ? ? ? ?"), false, 7, true);

		if (EngineVersion == 420)
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 40 83 79 04 00 48 8B DA 48 8B F9 75 23 E8 ? ? ? ? 48 85 C0 74 19 48 8B D3 48 8B C8 E8 ? ? ? ? 48"));
		else
		{
			ToStringAddr = FindPattern(_("40 53 48 83 EC 40 83 79 04 00 48 8B DA 75 19 E8 ? ? ? ? 48 8B C8 48 8B D3 E8 ? ? ? ?"));

			if (!ToStringAddr) // This means that we are in season 1 (i think).
			{
				ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 48 8B DA 4C 8B F1 E8 ? ? ? ? 4C 8B C8 41 8B 06 99"));

				if (ToStringAddr)
					EngineVersion = 416;
			}
		}

		FreeMemoryAddr = FindPattern(_("48 85 C9 74 1D 4C 8B 05 ? ? ? ? 4D 85 C0 0F 84 ? ? ? ? 49"));

		if (!FreeMemoryAddr)
			FreeMemoryAddr = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0C E8 ? ? ? ? 48 8B 0D ? ? ? ? 48"));

		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 48 63 41 0C 45 33 F6"));

		bOldObjects = true;
	}

	if (EngineVersion >= 421 && EngineVersion <= 424)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 30 83 79 04 00 48 8B DA 48 8B F9"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? ? ? ? 45 33 F6"));
	}

	if (EngineVersion >= 425 && EngineVersion < 500)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 30 8B 01 48 8B F1 44 8B 49 04 8B F8 C1 EF 10 48 8B DA 0F B7 C8 89 4D 24 89 7D 20 45 85 C9"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 8B 41 0C 45 33 F6"));
	}

	auto FnVerDouble = std::stod(FN_Version);

	if (FnVerDouble >= 5)
	{
		ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1 EB 03 48 8B ? 81 48 08 ? ? ? 40 49"), false, 7, true);
		FreeMemoryAddr = FindPattern(_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9"));
		bOldObjects = false;

		if (!ObjectsAddr)
			ObjectsAddr = FindPattern(_("48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1"), true, 3);
	}

	if (FnVerDouble >= 16.00) // 4.26.1
	{
		FreeMemoryAddr = FindPattern(_("48 85 C9 0F 84 ? ? ? ? 48 89 5C 24 ? 57 48 83 EC 20 48 8B 3D ? ? ? ? 48 8B D9 48"));

		if (FnVerDouble < 19.00)
		{
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 19 48 8B F2 0F B7 FB 4C 8B F1 E8 ? ? ? ? 44 8B C3 8D 1C 3F 49 C1 E8 10 33 FF 4A 03 5C C0 ? 41 8B 46 04"));
			ProcessEventAddr = FindPattern(_("40 55 53 56 57 41 54 41 56 41 57 48 81 EC"));

			if (!ToStringAddr)
				ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 19 33 ED 0F B7 01 48 8B FA C1 EB 10 4C"));
		}
	}

	// if (EngineVersion >= 500)
	if (FnVerDouble >= 19.00)
	{
		ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B"));
		ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45 33 ED"));

		if (!FreeMemoryAddr)
			FreeMemoryAddr = FindPattern(_("48 85 C9 0F 84 ? ? ? ? 53 48 83 EC 20 48 89 7C 24 ? 48 8B D9 48 8B 3D ? ? ? ? 48 85 FF"));

		// C3 S3

		if (!ToStringAddr)
			ToStringAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 8B 01 48 8B F2 8B"));

		if (!ProcessEventAddr)
			ProcessEventAddr = FindPattern(_("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45"));
	}

	if (!FreeMemoryAddr)
	{
		MessageBoxA(0, _("Failed to find FMemory::Free"), _("Fortnite"), MB_OK);
		return false;
	}

	FMemory::Free = decltype(FMemory::Free)(FreeMemoryAddr);

	toFree.FreeString();

	if (!ToStringAddr)
	{
		MessageBoxA(0, _("Failed to find FName::ToString"), _("Fortnite"), MB_OK);
		return false;
	}

	ToStringO = decltype(ToStringO)(ToStringAddr);

	if (!ProcessEventAddr)
	{
		MessageBoxA(0, _("Failed to find UObject::ProcessEvent"), _("Fortnite"), MB_OK);
		return false;
	}

	ProcessEventO = decltype(ProcessEventO)(ProcessEventAddr);

	if (!ObjectsAddr)
	{
		MessageBoxA(0, _("Failed to find FUObjectArray::ObjObjects"), _("Fortnite"), MB_OK);
		return false;
	}

	if (bOldObjects)
		OldObjects = decltype(OldObjects)(ObjectsAddr);
	else
		ObjObjects = decltype(ObjObjects)(ObjectsAddr);

	return true;
}

template <typename ReturnType = UObject>
static ReturnType* FindObject(const std::string& str, bool bIsEqual = false, bool bIsName = false)
{
	if (bIsName) bIsEqual = true;

	for (int32_t i = 0; i < (ObjObjects ? ObjObjects->Num() : OldObjects->Num()); i++)
	{
		auto Object = ObjObjects ? ObjObjects->GetObjectById(i) : OldObjects->GetObjectById(i);

		if (!Object) continue;

		auto ObjectName = bIsName ? Object->GetName() : Object->GetFullName();

		if (bIsEqual)
		{
			if (ObjectName == str)
				return (ReturnType*)Object;
		}
		else
		{
			if (ObjectName.contains(str))
				return (ReturnType*)Object;
		}
	}

	return nullptr;
}

static int GetOffset(UObject* Object, const std::string& MemberName)
{
	if (Object && !MemberName.contains(_(" ")))
	{
		auto Members = GetMembers(Object);

		for (auto& Member : Members)
		{
			if (Member->GetName() == MemberName)
				return ((Property*)Member)->GetOffset();
		}
	}
}

template <typename MemberType>
__forceinline MemberType* UObject::Member(const std::string& MemberName)
{
	// MemberName.erase(0, MemberName.find_last_of(".", MemberName.length() - 1) + 1); // This would be getting the short name of the member if you did like ObjectProperty /Script/stuff

	return (MemberType*)(__int64(this) + GetOffset(this, MemberName));
}