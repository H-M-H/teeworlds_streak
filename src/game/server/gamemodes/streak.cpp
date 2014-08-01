#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "streak.h"

CGameControllerStreak::CGameControllerStreak(class CGameContext *pGameServer)
: IGameController(pGameServer)
{

    if(str_comp_nocase(g_Config.m_SvGametype, "gStreak") == 0)
    {
        m_SubMode = SUBMODE_GRENADE;
        m_pGameType = "gStreak";
    }
    else if(str_comp_nocase(g_Config.m_SvGametype, "iStreak") == 0)
    {
        m_SubMode = SUBMODE_INSTA;
        m_pGameType = "iStreak";
    }
    else
    {
        m_SubMode = SUBMODE_VANILLA;
        m_pGameType = "Streak";
    }

    for(int i = 0; i < 5; i++)
        m_Spawntypes[i] = false;
    m_MaxLevels = 0;
    m_MaxSensfulLevel = 1;
}

void CGameControllerStreak::Tick()
{
    IGameController::Tick();

    int SubMode;

    if(str_comp_nocase(g_Config.m_SvGametype, "Streak") == 0)
        SubMode = SUBMODE_VANILLA;
    else if(str_comp_nocase(g_Config.m_SvGametype, "gStreak") == 0)
        SubMode = SUBMODE_GRENADE;
    else if(str_comp_nocase(g_Config.m_SvGametype, "iStreak") == 0)
        SubMode = SUBMODE_INSTA;
    else
        SubMode = m_SubMode;

    ChangeSubMode(SubMode);



    int NumPlayersPlaying = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
            NumPlayersPlaying++;
    }
    m_MaxSensfulLevel = clamp((NumPlayersPlaying+1)/2, 1, m_MaxLevels);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
        {
            if(GameServer()->m_apPlayers[i]->m_Level > m_MaxSensfulLevel)
                LevelDown(GameServer()->m_apPlayers[i], true);
            else if(GameServer()->m_apPlayers[i]->m_Level < m_MaxSensfulLevel && GameServer()->m_apPlayers[i]->m_Streak >= g_Config.m_SvStreakLen)
                LevelUp(GameServer()->m_apPlayers[i], true);
        }
    }
}

bool CGameControllerStreak::CanSpawn(int Team, vec2 *pOutPos, int Level)
{
    CSpawnEval Eval;

    // spectators can't spawn
    if(Team == TEAM_SPECTATORS)
        return false;

     int SpawnType = -1;
     int Num = 0;

     while(Num != Level && SpawnType < m_MaxLevels)
     {
         SpawnType++;
         if(m_Spawntypes[SpawnType])
             Num++;
     }

     EvaluateSpawnType(&Eval, SpawnType);


    *pOutPos = Eval.m_Pos;
    return Eval.m_Got;
}

bool CGameControllerStreak::OnEntity(int Index, vec2 Pos)
{
    int Type = -1;
    int SubType = 0;

    if(Index == ENTITY_SPAWN)
    {
        m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
        m_Spawntypes[0] = true;
    }
    else if(Index == ENTITY_SPAWN_RED)
    {
        m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
        m_Spawntypes[1] = true;
    }
    else if(Index == ENTITY_SPAWN_BLUE)
    {
        m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
        m_Spawntypes[2] = true;
    }
    else if(Index == ENTITY_SPAWN_YELLOW)
    {
        m_aaSpawnPoints[3][m_aNumSpawnPoints[3]++] = Pos;
        m_Spawntypes[3] = true;
    }
    else if(Index == ENTITY_SPAWN_GREEN)
    {
        m_aaSpawnPoints[4][m_aNumSpawnPoints[4]++] = Pos;
        m_Spawntypes[4] = true;
    }
    else if(Index == ENTITY_ARMOR_1)
        Type = POWERUP_ARMOR;
    else if(Index == ENTITY_HEALTH_1)
        Type = POWERUP_HEALTH;
    else if(Index == ENTITY_WEAPON_SHOTGUN)
    {
        Type = POWERUP_WEAPON;
        SubType = WEAPON_SHOTGUN;
    }
    else if(Index == ENTITY_WEAPON_GRENADE)
    {
        Type = POWERUP_WEAPON;
        SubType = WEAPON_GRENADE;
    }
    else if(Index == ENTITY_WEAPON_RIFLE)
    {
        Type = POWERUP_WEAPON;
        SubType = WEAPON_RIFLE;
    }
    else if(Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
    {
        Type = POWERUP_NINJA;
        SubType = WEAPON_NINJA;
    }

    m_MaxLevels = 0;
    for(int i = 0; i < 5 ; i++)
        m_MaxLevels += m_Spawntypes[i];

    if(m_SubMode != SUBMODE_VANILLA)
        return false;

    if(Type != -1)
    {
        CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType);
        pPickup->m_Pos = Pos;
        return true;
    }

    return false;
}

void CGameControllerStreak::PostReset()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i])
        {
            GameServer()->m_apPlayers[i]->Respawn();
            GameServer()->m_apPlayers[i]->m_Score = 0;
            GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
            GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
            GameServer()->m_apPlayers[i]->m_Level = 1;
            GameServer()->m_apPlayers[i]->m_Streak = 0;
            OnPlayerInfoChange(GameServer()->m_apPlayers[i]);
        }
    }
}

void CGameControllerStreak::OnPlayerInfoChange(class CPlayer *pP)
{
    const int aLevelColors[4] = {11468544, 5242624, 2948864, 65280};
    if(g_Config.m_SvColors)
    {
        if(pP->m_Level < 5)
        {
            pP->m_TeeInfos.m_UseCustomColor = 1;
            pP->m_TeeInfos.m_ColorBody = aLevelColors[pP->m_Level - 1];
            pP->m_TeeInfos.m_ColorFeet = aLevelColors[pP->m_Level - 1];
        }
        else
            pP->m_TeeInfos.m_UseCustomColor = 0;
    }
}

void CGameControllerStreak::OnCharacterSpawn(class CCharacter *pChr)
{
    if(m_SubMode == SUBMODE_GRENADE)
    {
        // default health
        pChr->IncreaseHealth(10);
        // endless grenade
        pChr->GiveWeapon(WEAPON_GRENADE, -1);
        pChr->SetWeapon(WEAPON_GRENADE);
    }
    else if (m_SubMode == SUBMODE_INSTA)
    {
        // default health
        pChr->IncreaseHealth(10);
        // endless rifle
        pChr->GiveWeapon(WEAPON_RIFLE, -1);
        pChr->SetWeapon(WEAPON_RIFLE);
    }
    else
    {
        // default health
        pChr->IncreaseHealth(10);

        // give default weapons
        pChr->GiveWeapon(WEAPON_HAMMER, -1);
        pChr->GiveWeapon(WEAPON_GUN, 10);
    }
}

int CGameControllerStreak::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
    // level down for Victim
    if(Weapon != WEAPON_GAME)
        LevelDown(pVictim->GetPlayer());

    // do scoreing
    if(!pKiller || Weapon == WEAPON_GAME)
        return 0;

    if(pKiller == pVictim->GetPlayer())
    {
        pVictim->GetPlayer()->m_Score--; // suicide
    }
    else
    {
        if(IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
            pKiller->m_Score--; // teamkill
        else
        {
            pKiller->m_Score += pKiller->m_Level; // normal kill
            pKiller->m_Streak++;
            if(pKiller->m_Level < m_MaxSensfulLevel && pKiller->m_Streak >= g_Config.m_SvStreakLen)
                LevelUp(pKiller);
        }
    }
    if(Weapon == WEAPON_SELF)
        pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;
    return 0;
}

void CGameControllerStreak::LevelUp(class CPlayer* pP, bool Game)
{
    if(pP->m_Level == m_MaxSensfulLevel)
    {
        return;
    }

    pP->m_Level++; // levelup \o/

    if(pP->m_Level > m_MaxSensfulLevel)
        pP->m_Level = m_MaxSensfulLevel;

    if(!Game)
        pP->m_Streak = 0;
    else
    {
        pP->m_Streak -= g_Config.m_SvStreakLen;
        if(pP->m_Streak < 0)
            pP->m_Streak = 0;
    }


    if(pP->GetCharacter())
        pP->GetCharacter()->ForceRespawn();
    OnPlayerInfoChange(pP);
}

void CGameControllerStreak::LevelDown(class CPlayer* pP, bool Game)
{
    if(pP->m_Level <= 1)
    {
        pP->m_Level = 1;
        pP->m_Streak = 0;
        return;
    }

    pP->m_Level -= g_Config.m_SvLevelLose;
    if(pP->m_Level < 1)
        pP->m_Level = 1;

    if(!Game)
        pP->m_Streak = 0;
    else
        pP->m_Streak += g_Config.m_SvStreakLen;


    if(pP->GetCharacter() && Game)
        pP->GetCharacter()->ForceRespawn();
    OnPlayerInfoChange(pP);
}

void CGameControllerStreak::ChangeSubMode(int Mode)
{
    if(Mode == m_SubMode)
        return;

    switch(Mode)
    {
        case SUBMODE_GRENADE:
        {
            if(m_SubMode == SUBMODE_VANILLA)
                RemoveAllPickups();
            SetWeaponsAll(WEAPON_GRENADE, -1);
            AdjustLivesAll();
            m_SubMode = SUBMODE_GRENADE;
            m_pGameType = "gStreak";
            break;
        }
        case SUBMODE_INSTA:
        {
            if(m_SubMode == SUBMODE_VANILLA)
                RemoveAllPickups();
            SetWeaponsAll(WEAPON_RIFLE, -1);
            AdjustLivesAll();
            m_SubMode = SUBMODE_INSTA;
            m_pGameType = "iStreak";
            break;
        }
        default:
        {
            if(m_SubMode == SUBMODE_GRENADE || m_SubMode == SUBMODE_INSTA)
            {
                m_SubMode = SUBMODE_VANILLA;
                GameServer()->InitEntities();
            }
            SetWeaponsAll(-1, -1);
            AdjustLivesAll();
            m_SubMode = SUBMODE_VANILLA;
            m_pGameType = "Streak";
        }
    }
}

void CGameControllerStreak::SetWeaponsAll(int Weapon, int Ammo)
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
        {
            if(Weapon == -1)
            {
                // remove all Weapons
                for(int j = 0; j < NUM_WEAPONS; j++)
                    GameServer()->m_apPlayers[i]->GetCharacter()->RemoveWeapon(j);

                // give gun and hammer
                GameServer()->m_apPlayers[i]->GetCharacter()->GiveWeapon(WEAPON_GUN, 10);
                GameServer()->m_apPlayers[i]->GetCharacter()->GiveWeapon(WEAPON_HAMMER, -1);
                GameServer()->m_apPlayers[i]->GetCharacter()->SetWeapon(WEAPON_GUN);
            }
            else
            {
                // remove all Weapons
                for(int j = 0; j < NUM_WEAPONS; j++)
                    GameServer()->m_apPlayers[i]->GetCharacter()->RemoveWeapon(j);

                // give specified Weapon
                GameServer()->m_apPlayers[i]->GetCharacter()->GiveWeapon(Weapon, Ammo);
                GameServer()->m_apPlayers[i]->GetCharacter()->SetWeapon(Weapon);
            }
        }
    }
}


void CGameControllerStreak::RemoveAllPickups()
{
    GameServer()->m_World.DestroyEntitiesType(CGameWorld::ENTTYPE_PICKUP);
}

void CGameControllerStreak::AdjustLivesAll()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
        {
            GameServer()->m_apPlayers[i]->GetCharacter()->IncreaseHealth(10);
            GameServer()->m_apPlayers[i]->GetCharacter()->IncreaseArmor(-10);
        }
    }
}
