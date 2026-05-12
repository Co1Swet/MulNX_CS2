#include "ConVarSystem.hpp"

#include <string.h>
#include <MulNXExtensions/WinExt/WinExt.hpp>

void* C_ConVarSystem::GetFirstCvarIterator(uint64_t& idx)const {
    static auto exec = IVClass::Assume(this->Address)->GetVFunc<void* (uint64_t&)>(12);
    return exec(idx);
}
void* C_ConVarSystem::GetNextCvarIterator(uint64_t& idx)const {
    static auto exec = IVClass::Assume(this->Address)->GetVFunc<void* (uint64_t&, uint64_t)>(13);
    return exec(idx, idx);
}
C_ConVar* C_ConVarSystem::GetCVarByIndex(uint64_t index)const {
    static auto exec = IVClass::Assume(this->Address)->GetVFunc<C_ConVar * (uint64_t)>(43);
    return exec(index);
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
		this->GetNextCvarIterator(i);
	}
	return nullptr;
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
		this->GetNextCvarIterator(i);
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
		this->GetNextCvarIterator(i);
	}
	return;
}

C_ConVar* C_ConVarSystem::GetCvar(const std::string& CvarName){
	C_ConVar* pCvar = this->GetCVarByName(CvarName.c_str());
	if (pCvar) {
		this->CvarMap[CvarName] = pCvar;
	}
	return pCvar;
}