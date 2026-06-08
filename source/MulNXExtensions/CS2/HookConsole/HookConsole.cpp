#include "HookConsole.hpp"


bool HookConsole::Init() {
    this->CS2Cmds.reserve(100);
    this->ISys()
        .SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {return this->OnTier0Load(msg);});

    return true;
}

void HookConsole::OnTier0Load(MulNX::Message& msg) {
    auto VEngineCvar007 = (uintptr_t)this->CS2->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("VEngineCvar007", nullptr);

    this->GetFirstCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&)>(12);
    this->GetNextCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&, uint64_t)>(13);
    this->GetCVarByIndex = IVClass::Assume(VEngineCvar007)->GetVFunc<C_ConVar * (uint64_t)>(43);

    this->hkVEngineCvar007_RegisterConCommand = MulNX::Hook::Create((uint8_t*)IVClass::Assume(VEngineCvar007)->GetVFuncPtr(44), [this](MulNX::Hook* hk, RegContext* ctx) {
        auto pOrigCmd = ctx->r8;

        this->RegisterCS2Cmd = [this, &hk, &ctx](std::string&& name, std::string&& help, std::function<void(CCommand*)>&& callback)->void {
            this->CS2Cmds.push_back({ std::move(name), std::move(help), std::move(callback) });
            auto& cmd = this->CS2Cmds.back();

            CCmd cmdForCS2(
                cmd.name.c_str(),
                cmd.help.c_str(),
                FCVAR_CLIENTDLL,
                &cmd.callback
            );
            ctx->r8 = (uint64_t)&cmdForCS2;
            hk->CallMaybeOrigin(5, ctx);

            };

        this->ISys().PublishSync("Hook/RegisterConCommand/RegisterOurCmd"_hash);

        ctx->r8 = pOrigCmd;
        hk->CallMaybeOrigin(5, ctx);
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnRegisterConCommand(hk, ctx);});
        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->hkVEngineCvar007_RegisterConCommand->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "VEngineCvar007::RegisterConCommand"));
}

MulNX::Hook::Then HookConsole::HandleOnRegisterConCommand(MulNX::Hook* hk, RegContext* ctx) {
    CCmd* pCmd = (CCmd*)ctx->r8;
    std::string_view name = pCmd->m_pszName;
    if (name == "playdemo") {
        auto pf = pCmd->m_pCommandCallback;
        this->hkPlaydemo = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto cmd = (CCommand*)ctx->rdx;
            auto str = std::string(cmd->pRawString);
            this->ISys().LogInfo(str);
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPlaydemo->Attach();

        return MulNX::Hook::Then::Continue;
    }


    return MulNX::Hook::Then::Continue;
}

C_ConVar* HookConsole::GetCVarByName(const char* var_name)const {
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
C_ConVar* HookConsole::GetCvar(const std::string& CvarName) {
    C_ConVar* pCvar = this->GetCVarByName(CvarName.c_str());
    return pCvar;
}

void HookConsole::UnlockHiddenCVars(int& Count)const {
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
void HookConsole::LockAllCvars(int& Count)const {
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