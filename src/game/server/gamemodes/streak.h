#ifndef STREAK_H
#define STREAK_H

class CGameControllerStreak : public IGameController
{
    // store which spawntypes are used (normal, red, blue, yellow, green)
    bool m_Spawntypes[5];
    // maximum level that can be reached on map
    int m_MaxLevels;
    // this depends on playernumber so there is always someone to fight against
    int m_MaxSensfulLevel;

public:
    CGameControllerStreak(class CGameContext *pGameServer);
    void Tick();
    bool CanSpawn(int Team, vec2 *pOutPos, int Level);
    bool OnEntity(int Index, vec2 Pos);
    void PostReset();
    void OnPlayerInfoChange(class CPlayer *pP);
    void OnCharacterSpawn(class CCharacter *pChr);
    int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
    void LevelUp(CPlayer *pP, bool Game = false);
    void LevelDown(CPlayer *pP, bool Game = false);

    int m_SubMode;
    enum
    {
        SUBMODE_VANILLA,
        SUBMODE_GRENADE,
        SUBMODE_INSTA
    };

    void ChangeSubMode(int Mode);
    void SetWeaponsAll(int Weapon, int Ammo);
    void RemoveAllPickups();
    void AdjustLivesAll();

    // getters
    int GetMaxLevel() { return m_MaxLevels; }
    int GetMaxSensfulLevel() { return m_MaxSensfulLevel; }
};

#endif // STREAK_H
