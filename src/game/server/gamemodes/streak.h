#ifndef STREAK_H
#define STREAK_H

class CGameControllerStreak : public IGameController
{
    // store which spawntypes are used (normal, red, blue, yellow, green)
    bool m_Spawntypes[5];
    // the arenas the players belong to by their level / wanted arenas
    int m_aWantedArenaPlayers[5];
    // maximum arena that can be reached on map
    int m_MaxArenas;
    // this depends on playernumber so there is always someone to fight against
    int m_MaxSensfulLevel;
    //
    bool m_UseArenas;

public:
    CGameControllerStreak(class CGameContext *pGameServer);
    void Tick();
    bool CanSpawn(int Team, vec2 *pOutPos, int ID);
    bool OnEntity(int Index, vec2 Pos);
    void PostReset();
    void OnPlayerInfoChange(class CPlayer *pP);
    void OnCharacterSpawn(class CCharacter *pChr);
    int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
    void DoWincheck();
    void LevelUp(CPlayer *pP, bool Game = false);
    void LevelDown(CPlayer *pP, bool Game = false);

    int m_SubMode;
    enum
    {
        SUBMODE_VANILLA,
        SUBMODE_GRENADE,
        SUBMODE_INSTA
    };

    void CheckSubMode();
    void ReFreshPlayerStats();
    void HandlePlayerNumChanges();
    void HandleWaiting();
    void SetWeaponsAll(int Weapon, int Ammo);
    void RemoveAllPickups();
    void AdjustLivesAll();
    int GetWantedArena(int Level);

    // getters
    int GetMaxLevels();
    int GetMaxSensfulLevel() { return m_MaxSensfulLevel; }
};

#endif // STREAK_H
