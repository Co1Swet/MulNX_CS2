#pragma once
#include <Intro/CSModuleBase.hpp>

//1到10为玩家，0为本地
class D_Player {
public:
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 EyePosition;
    DirectX::XMFLOAT3 Rotation;
    int HP;
    int Team;
    bool Alive;
    int IndexInEntityList;
    int IndexInMap;
};

class D_GameData {
public:
    D_Player Players[11];

};

class IEntityIterationModule;
class BackgroundEntityScan final :public CSModuleBase {
    bool Init()override;
    void Main();
public:
    std::vector<IEntityIterationModule*>ParticipateItCSModules{};

    D_GameData CS2EBGameData{};
    D_Player& GetPlayerMsg(int Index);
};