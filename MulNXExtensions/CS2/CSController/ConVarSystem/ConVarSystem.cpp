#include "ConVarSystem.hpp"

#include <string.h>
#include <MulNXExtensions/WinExt/WinExt.hpp>

bool C_ConVarSystem::Load(uintptr_t addr) {
    this->GetFirstCvarIterator = IVClass::Assume(addr)->GetVFunc<void* (uint64_t&)>(12);
    this->GetNextCvarIterator = IVClass::Assume(addr)->GetVFunc<void* (uint64_t&, uint64_t)>(13);
    this->GetCVarByIndex = IVClass::Assume(addr)->GetVFunc<C_ConVar * (uint64_t)>(43);

    return true;
}

C_ConVar* C_ConVarSystem::GetCVarByName(const char* var_name)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pCvar = nullptr;
		pCvar = this->GetCVarByIndex(i);
		if (strcmp(pCvar->szName, var_name) == 0) {
			return pCvar;
		}
        this->GetNextCvarIterator(i, i);
	}
	return nullptr;
}
C_ConVar* C_ConVarSystem::GetCvar(const std::string& CvarName) {
    C_ConVar* pCvar = this->GetCVarByName(CvarName.c_str());
    return pCvar;
}

void C_ConVarSystem::UnlockHiddenCVars(int& Count)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pConVar = this->GetCVarByIndex(i);
		if (pConVar) {
			if (pConVar->IsHidden()) {
				pConVar->Unhide();
				++Count;
			}
		}
		this->GetNextCvarIterator(i,i);
	}
	return;
}
void C_ConVarSystem::LockAllCvars(int& Count)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pConVar = this->GetCVarByIndex(i);
		if (pConVar) {
			if (!pConVar->IsHidden()) {
				pConVar->Hide();
				++Count;
			}
		}
		this->GetNextCvarIterator(i,i);
	}
	return;
}