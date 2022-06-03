#include "structs.h"

struct Var // is this memory efficent? no, i do not care though.
{
	int val = 0;
	FString str;
	void* ptr = nullptr;

	Var(int Val, void* Ptr) : val(Val), ptr(Ptr) {}

	Var(void* Ptr) : ptr(Ptr) {}

	Var(int Val) : val(Val) {}

	Var() {}
	
	int GetUsedVariable() const
	{
		if (ptr)
			return 2;
		else if (val)
			return 0;
		else if (str.Data.GetData())
			return 1;

		return -1;
	}
};

template <typename Type>
void Set(Type Value, void* Block, size_t Addition)
{
	auto block = (Type*)((char*)Block + Addition);

	if (block)
	{
		*block = Value;
	}
}

void SetAll(const std::vector<Var>& Vars, void* block, size_t Addition = 0) // i do not remember why I put the addition here tbh
{
	if (!block)
		return;

	for (auto& var : Vars)
	{
		switch (var.GetUsedVariable())
		{
		case 0: // val (int)
			Set(var.val, block, Addition);
			Addition += sizeof(var.val);
			break;
		case 1: // str (FString)
			Set(var.str, block, Addition);
			Addition += sizeof(var.str);
			break;
		case 2: // ptr (void*)
			Set(var.ptr, block, Addition);
			Addition += sizeof(var.ptr);
			break;
		}
	}
}

struct Finder
{
	UObject* m_Object = nullptr;
	// bool bSucceededLastOperation = false;

	Finder(const std::string& BaseObject) : m_Object(FindObject(BaseObject)) {}
	
	UObject* operator->() const { return m_Object; }

	Finder& operator[](const std::string& ChildObject)
	{
		m_Object = *m_Object->Member<UObject*>(ChildObject);
		return *this;
	}

	template<typename ArrayType>
	Finder& ArrayAt(const std::string& ChildArray, int element) // since we are unable to set m_Object to anything but a object, we have to use a workaround
	{
		auto Array = m_Object->Member<TArray<ArrayType>>(ChildArray);

		if (Array) // && *Array)
		{
			m_Object = Array->At(element);
		}

		return *this;
	}

	// addition = if you have like a returnvalue or something. Returns nullptr or Additionmemory if any
	// insteadof addition we can do a template and then get the size from that and then return that. but i'll add later
	void* Call(const std::string& FunctionName, const std::vector<Var>& Vars, size_t Addition = 0, size_t* OutSize = nullptr)
	{
		if (Vars.size() == 0)
		{
			static auto fn = m_Object->Function(FunctionName);

			if (fn)
				return m_Object->ProcessEvent(fn, nullptr);

			return nullptr;
		}

		void* AdditionMemory = nullptr;
		std::vector<void*> Mem;

		// std::vector<size_t> Sizes;
		size_t allSizesAdded = 0;

		for (auto& Var : Vars)
		{
			int varSize = 0;
			switch (Var.GetUsedVariable())
			{
			case 0: // val (int)
				varSize = sizeof(Var.val);
				break;
			case 1: // str (FString)
				varSize = sizeof(Var.str);
				break;
			case 2: // ptr (void*)
				varSize = sizeof(Var.ptr);
				break;
			default:
				break;
			}

			// Sizes.push_back(varSize);
			allSizesAdded += varSize;
		}

		auto Memory = malloc(allSizesAdded + Addition);

		if (Addition)
			AdditionMemory = (void*)(__int64(Memory) - Addition);

		/* for (int i = 0; i < Sizes.size(); i++)
		{
			auto Size = Sizes[i];

			auto newMem = malloc(Sizes[i]);

			if (OutSize)
				*OutSize += Size;

			Mem.push_back(newMem);

			if (i == Sizes.size() - 1 && Addition)
				AdditionMemory = malloc(Addition);
			// Mem.push_back(malloc(Addition)); // we cannjot do this because it will be freed
		} */

		if (Memory)
		{
			SetAll(Vars, Memory);
			static auto fn = m_Object->Function(FunctionName);

			std::cout << "Calling\n";

			if (fn)
				m_Object->ProcessEvent(fn, Memory);

			std::cout << "Called\n";

			std::cout << Memory << "\n";
			std::cout << allSizesAdded << '\n';
			std::cout << "Addition: " << Addition << '\n';
			std::cout << AdditionMemory << '\n';
			std::cout << "Full Name of Ret: " << ((UObject*)AdditionMemory)->GetFullName() << '\n';

			// free(Memory);
		}

		if (AdditionMemory)
			return AdditionMemory;

		else
			return nullptr;
	}
};