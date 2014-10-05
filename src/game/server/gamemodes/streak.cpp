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
    {
        m_Spawntypes[i] = false;
        m_aWantedArenaPlayers[i] = 0;
    }
    m_MaxArenas = 0;
    m_MaxSensfulLevel = 1;

    m_UseArenas = g_Config.m_SvUseArenas;
}

void CGameControllerStreak::Tick()
{
    IGameController::Tick();

    CheckSubMode();

    ReFreshPlayerStats();

    if(m_UseArenas != g_Config.m_SvUseArenas)
    {
        m_UseArenas = g_Config.m_SvUseArenas;
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
                GameServer()->m_apPlayers[i]->GetCharacter()->ForceRespawn();
        }
    }

    HandlePlayerNumChanges();

    HandleWaiting();
}

bool CGameControllerStreak::CanSpawn(int Team, vec2 *pOutPos, int ID)
{
    CSpawnEval Eval;

    // spectators can't spawn
    if(Team == TEAM_SPECTATORS)
        return false;

    if(g_Config.m_SvUseArenas)
    {
        int Arena = GetWantedArena(GameServer()->m_apPlayers[ID]->m_Level);
        if(m_aWantedArenaPlayers[Arena] > 1 || !g_Config.m_SvWaitFirstArena)
        {
            EvaluateSpawnType(&Eval, Arena);
            if(Eval.m_Got)
                GameServer()->m_apPlayers[ID]->m_Arena = Arena;
        }
        else
        {
            EvaluateSpawnType(&Eval, GetWantedArena(1));
            if(Eval.m_Got)
            {
                GameServer()->m_apPlayers[ID]->m_Arena = GetWantedArena(1);

                // send message if player is not waiting yet
                if(m_aWantedArenaPlayers[GetWantedArena(1)] > 1 && !GameServer()->m_apPlayers[ID]->m_Waiting)
                    GameServer()->SendChatTarget(ID, "Waiting in first arena until enemies reach your arena");
            }
        }
        *pOutPos = Eval.m_Pos;
        return Eval.m_Got;
    }
    else
    {
        EvaluateSpawnType(&Eval, 0);
        if(Eval.m_Got)
            GameServer()->m_apPlayers[ID]->m_Arena = 0;
        EvaluateSpawnType(&Eval, 1);
        if(Eval.m_Got)
            GameServer()->m_apPlayers[ID]->m_Arena = 1;
        EvaluateSpawnType(&Eval, 2);
        if(Eval.m_Got)
            GameServer()->m_apPlayers[ID]->m_Arena = 2;

        *pOutPos = Eval.m_Pos;
        return Eval.m_Got;
    }
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

    m_MaxArenas = 0;
    for(int i = 0; i < 5 ; i++)
        m_MaxArenas += m_Spawntypes[i];

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
    const int aLevelColors[6] = {11468544, 5242624, 2948864, 65280, 16777215, 0};
    if(g_Config.m_SvColors)
    {

        pP->m_TeeInfos.m_UseCustomColor = 1;
        if(pP->m_Streak >= g_Config.m_SvStreakLen && g_Config.m_SvEndOnLevel)
        {
            pP->m_TeeInfos.m_ColorBody = aLevelColors[pP->m_Level];
            pP->m_TeeInfos.m_ColorFeet = aLevelColors[pP->m_Level];
        }
        else if(pP->m_Streak + 1 >= g_Config.m_SvStreakLen)
        {
            pP->m_TeeInfos.m_ColorBody = aLevelColors[pP->m_Level - 1];
            pP->m_TeeInfos.m_ColorFeet = aLevelColors[pP->m_Level];
        }
        else
        {
            pP->m_TeeInfos.m_ColorBody = aLevelColors[pP->m_Level - 1];
            pP->m_TeeInfos.m_ColorFeet = aLevelColors[pP->m_Level - 1];
        }
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

    pChr->GetPlayer()->m_Waiting = pChr->GetPlayer()->m_WantedArena != pChr->GetPlayer()->m_Arena;
}

int CGameControllerStreak::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
    // do scoreing
    if(!pKiller || Weapon == WEAPON_GAME)
        return 0;

    if(pKiller == pVictim->GetPlayer())
    {
        pVictim->GetPlayer()->m_Score--; // suicide
        LevelDown(pVictim->GetPlayer());
    }
    else
    {
        if(g_Config.m_SvUseArenas)
        {
            if(pKiller->m_Arena == pKiller->m_WantedArena && pKiller->m_Level == pVictim->GetPlayer()->m_Level)
            {
                pKiller->m_Score += pKiller->m_Level; // normal kill
                pKiller->m_Streak++;
            }
            else if(pVictim->GetPlayer()->m_Arena == pVictim->GetPlayer()->m_WantedArena)
            {
                pKiller->m_Score += pVictim->GetPlayer()->m_Level; // killer died but had a projectile that killed his killer
            }
            else if(pVictim->GetPlayer()->m_Arena != pVictim->GetPlayer()->m_WantedArena)
            {
                pKiller->m_Score++; // Killer killed player with higher level
                pKiller->m_Streak++;
            }
            else
                pKiller->m_Score++; // Killer is from a higher level - don't give waiting players such a huge advantage
        }
        else
        {
            if(pKiller->m_Level <= pVictim->GetPlayer()->m_Level) // same or lower level
            {
                pKiller->m_Score += pKiller->m_Level; // normal kill
                pKiller->m_Streak++;
            }
            else
               pKiller->m_Score++;
        }

        // level down for Victim
        if((pVictim->GetPlayer()->m_Arena == pVictim->GetPlayer()->m_WantedArena && g_Config.m_SvUseArenas) || (!g_Config.m_SvUseArenas && pKiller->m_Level == pVictim->GetPlayer()->m_Level))
            LevelDown(pVictim->GetPlayer());

        if(pKiller->m_Level < m_MaxSensfulLevel && pKiller->m_Streak >= g_Config.m_SvStreakLen)
            LevelUp(pKiller);
    }
    if(Weapon == WEAPON_SELF)
        pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

    OnPlayerInfoChange(pVictim->GetPlayer());
    OnPlayerInfoChange(pKiller);

    return 0;
}

void CGameControllerStreak::DoWincheck()
{
    if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
    {
        // gather some stats
        int Topscore = 0;
        int TopscoreCount = 0;
        int LevelEndReached = -1;
        int TopScorePlayer = -1;
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(GameServer()->m_apPlayers[i])
            {
                if(GameServer()->m_apPlayers[i]->m_Score > Topscore)
                {
                    Topscore = GameServer()->m_apPlayers[i]->m_Score;
                    TopscoreCount = 1;
                    TopScorePlayer = i;
                }
                else if(GameServer()->m_apPlayers[i]->m_Score == Topscore)
                    TopscoreCount++;

                if(GameServer()->m_apPlayers[i]->m_Level >= m_MaxSensfulLevel && GameServer()->m_apPlayers[i]->m_Streak >= g_Config.m_SvStreakLen && g_Config.m_SvEndOnLevel)
                    LevelEndReached = i;
            }
        }

        // check score win condition
        if((g_Config.m_SvScorelimit > 0 && Topscore >= g_Config.m_SvScorelimit) ||
            (g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60))
        {
            if(TopscoreCount == 1)
            {
                char aBuf[256];
                str_format(aBuf, sizeof(aBuf), "'%s' wins this round !", Server()->ClientName(TopScorePlayer));
                GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
                GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "chat", aBuf);
                EndRound();
            }
            else
                m_SuddenDeath = 1;
        }
        else if(LevelEndReached != -1)
        {
            char aBuf[256];
            str_format(aBuf, sizeof(aBuf), "'%s' wins this round !", Server()->ClientName(LevelEndReached));
            GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
            GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "chat", aBuf);
            EndRound();
        }
    }
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


    if(pP->GetCharacter() && g_Config.m_SvUseArenas)
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

    if(pP->GetCharacter() && Game && g_Config.m_SvUseArenas)
        pP->GetCharacter()->ForceRespawn();
    OnPlayerInfoChange(pP);
}

void CGameControllerStreak::CheckSubMode()
{
    int SubMode;

    if(str_comp_nocase(g_Config.m_SvGametype, "Streak") == 0)
        SubMode = SUBMODE_VANILLA;
    else if(str_comp_nocase(g_Config.m_SvGametype, "gStreak") == 0)
        SubMode = SUBMODE_GRENADE;
    else if(str_comp_nocase(g_Config.m_SvGametype, "iStreak") == 0)
        SubMode = SUBMODE_INSTA;
    else
        SubMode = m_SubMode;

    if(SubMode == m_SubMode)
        return;

    switch(SubMode)
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

void CGameControllerStreak::ReFreshPlayerStats()
{
    for(int i = 0; i < 5; i++)
        m_aWantedArenaPlayers[i] = 0;

    int NumPlayersPlaying = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
        {
            NumPlayersPlaying++;

            int Arena = GetWantedArena(GameServer()->m_apPlayers[i]->m_Level);
            m_aWantedArenaPlayers[Arena]++;
            GameServer()->m_apPlayers[i]->m_WantedArena = Arena;
        }
    }
    m_MaxSensfulLevel = clamp((NumPlayersPlaying+1)/2, 1, GetMaxLevels());
}

void CGameControllerStreak::HandlePlayerNumChanges()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
        {
            // check players ingame and handle maxsensfullevel changes
            if(GameServer()->m_apPlayers[i]->m_Level > m_MaxSensfulLevel)
                LevelDown(GameServer()->m_apPlayers[i], true);
            else if(GameServer()->m_apPlayers[i]->m_Level < m_MaxSensfulLevel && GameServer()->m_apPlayers[i]->m_Streak >= g_Config.m_SvStreakLen)
                LevelUp(GameServer()->m_apPlayers[i], true);
        }
    }
}

void CGameControllerStreak::HandleWaiting()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter() && g_Config.m_SvUseArenas)
        {
            // handle waiting for enemies in same arena
            if(g_Config.m_SvWaitFirstArena)
            {
                // waiting but another player reached same arena
                if((GameServer()->m_apPlayers[i]->m_Waiting && m_aWantedArenaPlayers[GameServer()->m_apPlayers[i]->m_WantedArena] > 1) ||
                        // not waiting but arena out of enemies
                        (!GameServer()->m_apPlayers[i]->m_Waiting // not waiting
                         && m_aWantedArenaPlayers[GameServer()->m_apPlayers[i]->m_WantedArena] <= 1 // no players in own arena
                         && GameServer()->m_apPlayers[i]->m_WantedArena != GetWantedArena(1))) // if arena is one no respawn
                {
                    GameServer()->m_apPlayers[i]->GetCharacter()->ForceRespawn();
                }
            }
            // waiting but waiting turned off --> respawn
            else if(GameServer()->m_apPlayers[i]->m_Waiting)
                GameServer()->m_apPlayers[i]->GetCharacter()->ForceRespawn();
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

int CGameControllerStreak::GetWantedArena(int Level)
{
    int SpawnType = -1;
    int Num = 0;

    while(Num != Level && SpawnType < m_MaxArenas)
    {
        SpawnType++;
        if(m_Spawntypes[SpawnType])
            Num++;
    }
    return SpawnType;
}

int CGameControllerStreak::GetMaxLevels()
{
     return g_Config.m_SvUseArenas ? (m_MaxArenas < g_Config.m_SvMaxLevel ? m_MaxArenas : g_Config.m_SvMaxLevel) : g_Config.m_SvMaxLevel;
}
