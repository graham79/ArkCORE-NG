/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2011-2014 ArkCORE <http://www.arkania.net/> 
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GossipDef.h"
#include "SocialMgr.h"

void WorldSession::HandleGuildQueryOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_QUERY");

    uint64 guildGuid, playerGuid;
    recvPacket >> guildGuid >> playerGuid;

    // If guild doesn't exist or player is not part of the guild send error
    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(playerGuid))
            guild->HandleQuery(this);
}

void WorldSession::HandleGuildCreateOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_CREATE");

    std::string name;
    recvPacket >> name;

    if (!GetPlayer()->GetGuildId())             // Player cannot be in guild
    {
        Guild* guild = new Guild();
        if (guild->Create(GetPlayer(), name))
            sGuildMgr->AddGuild(guild);
        else
            delete guild;
    }
}

void WorldSession::HandleGuildInviteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_INVITE");

    uint32 nameLength = recvPacket.ReadBits(7);
    std::string invitedName = recvPacket.ReadString(nameLength);

    if (normalizePlayerName(invitedName))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleInviteMember(this, invitedName);
}

void WorldSession::HandleGuildRemoveOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_REMOVE");

    ObjectGuid playerGuid;

    playerGuid[6] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    playerGuid[1] = recvPacket.ReadBit();
    playerGuid[3] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();
    playerGuid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(playerGuid[2]);
    recvPacket.ReadByteSeq(playerGuid[6]);
    recvPacket.ReadByteSeq(playerGuid[5]);
    recvPacket.ReadByteSeq(playerGuid[7]);
    recvPacket.ReadByteSeq(playerGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[4]);
    recvPacket.ReadByteSeq(playerGuid[3]);
    recvPacket.ReadByteSeq(playerGuid[0]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleRemoveMember(this, playerGuid);
}

void WorldSession::HandleGuildAcceptOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ACCEPT");
    // Player cannot be in guild
    if (!GetPlayer()->GetGuildId())
        // Guild where player was invited must exist
        if (Guild* guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildIdInvited()))
            guild->HandleAcceptMember(this);
}

void WorldSession::HandleGuildDeclineOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DECLINE");

    GetPlayer()->SetGuildIdInvited(0);
    GetPlayer()->SetInGuild(0);
}

void WorldSession::HandleGuildRosterOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ROSTER");
    recvPacket.rfinish();

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleRoster(this);
    else
        Guild::SendCommandResult(this, GUILD_COMMAND_ROSTER, ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

void WorldSession::HandleGuildPromoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_PROMOTE");

    ObjectGuid targetGuid;

    targetGuid[7] = recvPacket.ReadBit();
    targetGuid[2] = recvPacket.ReadBit();
    targetGuid[5] = recvPacket.ReadBit();
    targetGuid[6] = recvPacket.ReadBit();
    targetGuid[1] = recvPacket.ReadBit();
    targetGuid[0] = recvPacket.ReadBit();
    targetGuid[3] = recvPacket.ReadBit();
    targetGuid[4] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(targetGuid[0]);
    recvPacket.ReadByteSeq(targetGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[2]);
    recvPacket.ReadByteSeq(targetGuid[3]);
    recvPacket.ReadByteSeq(targetGuid[6]);
    recvPacket.ReadByteSeq(targetGuid[4]);
    recvPacket.ReadByteSeq(targetGuid[1]);
    recvPacket.ReadByteSeq(targetGuid[7]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleUpdateMemberRank(this, targetGuid, false);
}

void WorldSession::HandleGuildDemoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DEMOTE");

    ObjectGuid targetGuid;

    targetGuid[7] = recvPacket.ReadBit();
    targetGuid[1] = recvPacket.ReadBit();
    targetGuid[5] = recvPacket.ReadBit();
    targetGuid[6] = recvPacket.ReadBit();
    targetGuid[2] = recvPacket.ReadBit();
    targetGuid[3] = recvPacket.ReadBit();
    targetGuid[0] = recvPacket.ReadBit();
    targetGuid[4] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(targetGuid[1]);
    recvPacket.ReadByteSeq(targetGuid[2]);
    recvPacket.ReadByteSeq(targetGuid[7]);
    recvPacket.ReadByteSeq(targetGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[6]);
    recvPacket.ReadByteSeq(targetGuid[0]);
    recvPacket.ReadByteSeq(targetGuid[4]);
    recvPacket.ReadByteSeq(targetGuid[3]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleUpdateMemberRank(this, targetGuid, true);
}

void WorldSession::HandleGuildAssignRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ASSIGN_MEMBER_RANK");

    ObjectGuid targetGuid;
    ObjectGuid setterGuid;

    uint32 rankId;
    recvPacket >> rankId;

    targetGuid[1] = recvPacket.ReadBit();
    targetGuid[7] = recvPacket.ReadBit();
    setterGuid[4] = recvPacket.ReadBit();
    setterGuid[2] = recvPacket.ReadBit();
    targetGuid[4] = recvPacket.ReadBit();
    targetGuid[5] = recvPacket.ReadBit();
    targetGuid[6] = recvPacket.ReadBit();
    setterGuid[1] = recvPacket.ReadBit();
    setterGuid[7] = recvPacket.ReadBit();
    targetGuid[2] = recvPacket.ReadBit();
    targetGuid[3] = recvPacket.ReadBit();
    targetGuid[0] = recvPacket.ReadBit();
    setterGuid[6] = recvPacket.ReadBit();
    setterGuid[3] = recvPacket.ReadBit();
    setterGuid[0] = recvPacket.ReadBit();
    setterGuid[5] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(targetGuid[0]);
    recvPacket.ReadByteSeq(setterGuid[1]);
    recvPacket.ReadByteSeq(setterGuid[3]);
    recvPacket.ReadByteSeq(setterGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[7]);
    recvPacket.ReadByteSeq(targetGuid[3]);
    recvPacket.ReadByteSeq(setterGuid[0]);
    recvPacket.ReadByteSeq(targetGuid[1]);
    recvPacket.ReadByteSeq(setterGuid[6]);
    recvPacket.ReadByteSeq(targetGuid[2]);
    recvPacket.ReadByteSeq(targetGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[4]);
    recvPacket.ReadByteSeq(setterGuid[2]);
    recvPacket.ReadByteSeq(setterGuid[4]);
    recvPacket.ReadByteSeq(targetGuid[6]);
    recvPacket.ReadByteSeq(setterGuid[7]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetMemberRank(this, targetGuid, setterGuid, rankId);
}

void WorldSession::HandleGuildLeaveOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_LEAVE");

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleLeaveMember(this);
}

void WorldSession::HandleGuildDisbandOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DISBAND");

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleDisband(this);
}

void WorldSession::HandleGuildMOTDOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_MOTD");

    uint32 motdLength = recvPacket.ReadBits(11);
    std::string motd = recvPacket.ReadString(motdLength);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetMOTD(this, motd);
}

void WorldSession::HandleGuildSetNoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_SET_NOTE");

    ObjectGuid playerGuid;

    playerGuid[1] = recvPacket.ReadBit();
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[3] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();
    bool type     = recvPacket.ReadBit();      // 0 == Officer, 1 == Public
    playerGuid[6] = recvPacket.ReadBit();
    uint32 noteLength = recvPacket.ReadBits(8);
    playerGuid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(playerGuid[4]);
    recvPacket.ReadByteSeq(playerGuid[5]);
    recvPacket.ReadByteSeq(playerGuid[0]);
    recvPacket.ReadByteSeq(playerGuid[3]);
    recvPacket.ReadByteSeq(playerGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[6]);
    recvPacket.ReadByteSeq(playerGuid[7]);
    std::string note = recvPacket.ReadString(noteLength);
    recvPacket.ReadByteSeq(playerGuid[2]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetMemberNote(this, note, playerGuid, type);
}

void WorldSession::HandleGuildQueryRanksOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_QUERY_RANKS");

    ObjectGuid guildGuid;

    guildGuid[2] = recvData.ReadBit();
    guildGuid[3] = recvData.ReadBit();
    guildGuid[0] = recvData.ReadBit();
    guildGuid[6] = recvData.ReadBit();
    guildGuid[4] = recvData.ReadBit();
    guildGuid[7] = recvData.ReadBit();
    guildGuid[5] = recvData.ReadBit();
    guildGuid[1] = recvData.ReadBit();

    recvData.ReadByteSeq(guildGuid[3]);
    recvData.ReadByteSeq(guildGuid[4]);
    recvData.ReadByteSeq(guildGuid[5]);
    recvData.ReadByteSeq(guildGuid[7]);
    recvData.ReadByteSeq(guildGuid[1]);
    recvData.ReadByteSeq(guildGuid[0]);
    recvData.ReadByteSeq(guildGuid[6]);
    recvData.ReadByteSeq(guildGuid[2]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->SendGuildRankInfo(this);
}

void WorldSession::HandleGuildAddRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ADD_RANK");

    uint32 rankId;
    recvPacket >> rankId;

    uint32 length = recvPacket.ReadBits(7);
    std::string rankName = recvPacket.ReadString(length);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleAddNewRank(this, rankName);
}

void WorldSession::HandleGuildDelRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_DEL_RANK");

    uint32 rankId;
    recvPacket >> rankId;

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleRemoveRank(this, rankId);
}

void WorldSession::HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_INFO_TEXT");

    uint32 length = recvPacket.ReadBits(12);
    std::string info = recvPacket.ReadString(length);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetInfo(this, info);
}

void WorldSession::HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received MSG_SAVE_GUILD_EMBLEM");

    uint64 vendorGuid;
    recvPacket >> vendorGuid;

    EmblemInfo emblemInfo;
    emblemInfo.ReadPacket(recvPacket);

    if (GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_TABARDDESIGNER))
    {
        // Remove fake death
        if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
            GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleSetEmblem(this, emblemInfo);
        else
            Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_NOGUILD); // "You are not part of a guild!";
    }
    else
    {
        Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_INVALIDVENDOR); // "That's not an emblem vendor!"
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: HandleSaveGuildEmblemOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(vendorGuid));
    }
}

void WorldSession::HandleGuildEventLogQueryOpcode(WorldPacket& /* recvPacket */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_EVENT_LOG_QUERY)");

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendEventLog(this);
}

void WorldSession::HandleGuildBankMoneyWithdrawn(WorldPacket & /* recvData */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_MONEY_WITHDRAWN_QUERY)");

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendMoneyInfo(this);
}

void WorldSession::HandleGuildPermissions(WorldPacket& /* recvData */)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_PERMISSIONS)");

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendPermissions(this);
}

// Called when clicking on Guild bank gameobject
void WorldSession::HandleGuildBankerActivate(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANKER_ACTIVATE)");

    uint64 guid;
    bool sendAllSlots;
    recvData >> guid >> sendAllSlots;

    GameObject const* const go = GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_GUILD_BANK);
    if (!go)
        return;

    Guild* const guild = GetPlayer()->GetGuild();
    if (!guild)
    {
        Guild::SendCommandResult(this, GUILD_COMMAND_VIEW_TAB, ERR_GUILD_PLAYER_NOT_IN_GUILD);
        return;
    }

    guild->SendBankList(this, 0, true, true);
}

// Called when opening guild bank tab only (first one)
void WorldSession::HandleGuildBankQueryTab(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_QUERY_TAB)");

    uint64 guid;
    uint8 tabId;
    bool sendAllSlots;

    recvData >> guid >> tabId >> sendAllSlots;

    if (GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->SendBankList(this, tabId, true, false);
}

void WorldSession::HandleGuildBankDepositMoney(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_DEPOSIT_MONEY)");

    uint64 guid;
    uint64 money;
    recvData >> guid >> money;

    if (GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (money && GetPlayer()->HasEnoughMoney(money))
            if (Guild* guild = GetPlayer()->GetGuild())
                guild->HandleMemberDepositMoney(this, money);
}

void WorldSession::HandleGuildBankWithdrawMoney(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_WITHDRAW_MONEY)");

    uint64 guid;
    uint64 money;
    recvData >> guid >> money;

    if (money && GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleMemberWithdrawMoney(this, money);
}

void WorldSession::HandleGuildBankSwapItems(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_SWAP_ITEMS)");

    uint64 GoGuid;
    recvData >> GoGuid;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
    {
        recvData.rfinish();                   // Prevent additional spam at rejected packet
        return;
    }

    Guild* guild = GetPlayer()->GetGuild();
    if (!guild)
    {
        recvData.rfinish();                   // Prevent additional spam at rejected packet
        return;
    }

    uint8 bankToBank;
    recvData >> bankToBank;

    uint8 tabId;
    uint8 slotId;
    uint32 itemEntry;
    uint32 splitedAmount = 0;

    if (bankToBank)
    {
        uint8 destTabId;
        recvData >> destTabId;

        uint8 destSlotId;
        recvData >> destSlotId;

        uint32 destItemEntry;
        recvData >> destItemEntry;

        recvData >> tabId;
        recvData >> slotId;
        recvData >> itemEntry;
        recvData.read_skip<uint8>();                       // Always 0
        recvData >> splitedAmount;

        guild->SwapItems(GetPlayer(), tabId, slotId, destTabId, destSlotId, splitedAmount);
    }
    else
    {
        uint8 playerBag = NULL_BAG;
        uint8 playerSlotId = NULL_SLOT;
        uint8 toChar = 1;

        recvData >> tabId;
        recvData >> slotId;
        recvData >> itemEntry;

        uint8 autoStore;
        recvData >> autoStore;
        if (autoStore)
        {
            recvData.read_skip<uint32>();                  // autoStoreCount
            recvData.read_skip<uint8>();                   // ToChar (?), always and expected to be 1 (autostore only triggered in Bank -> Char)
            recvData.read_skip<uint32>();                  // Always 0
        }
        else
        {
            recvData >> playerBag;
            recvData >> playerSlotId;
            recvData >> toChar;
            recvData >> splitedAmount;
        }

        // Player <-> Bank
        // Allow to work with inventory only
        if (!Player::IsInventoryPos(playerBag, playerSlotId) && !(playerBag == NULL_BAG && playerSlotId == NULL_SLOT))
            GetPlayer()->SendEquipError(EQUIP_ERR_INTERNAL_BAG_ERROR, NULL);
        else
            guild->SwapItemsWithInventory(GetPlayer(), toChar, tabId, slotId, playerBag, playerSlotId, splitedAmount);
    }
}

void WorldSession::HandleGuildBankBuyTab(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_BUY_TAB)");

    uint64 GoGuid;
    recvData >> GoGuid;

    uint8 tabId;
    recvData >> tabId;

    if (!GoGuid || GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* guild = GetPlayer()->GetGuild())
            guild->HandleBuyBankTab(this, tabId);
}

void WorldSession::HandleGuildBankUpdateTab(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (CMSG_GUILD_BANK_UPDATE_TAB)");

    uint64 guid;
    uint8 tabId;
    std::string name, icon;

    recvData >> guid >> tabId >> name >> icon;

    if (!name.empty() && !icon.empty())
        if (GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_GUILD_BANK))
            if (Guild* guild = GetPlayer()->GetGuild())
                guild->HandleSetBankTabInfo(this, tabId, name, icon);
}

void WorldSession::HandleGuildBankLogQuery(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received (MSG_GUILD_BANK_LOG_QUERY)");

    uint32 tabId;
    recvData >> tabId;

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendBankLog(this, tabId);
}

void WorldSession::HandleQueryGuildBankTabText(WorldPacket &recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_BANK_QUERY_TEXT");

    uint8 tabId;
    recvData >> tabId;

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendBankTabText(this, tabId);
}

void WorldSession::HandleSetGuildBankTabText(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_SET_GUILD_BANK_TEXT");

    uint32 tabId;
    recvData >> tabId;

    uint32 textLen = recvData.ReadBits(14);
    std::string text = recvData.ReadString(textLen);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SetBankTabText(tabId, text);
}

void WorldSession::HandleGuildQueryXPOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_QUERY_GUILD_XP");

    ObjectGuid guildGuid;

    guildGuid[2] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();
    guildGuid[0] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[2]);
    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[6]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[4]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->SendGuildXP(this);
}

void WorldSession::HandleGuildSetRankPermissionsOpcode(WorldPacket& recvPacket)
{
    Guild* guild = GetPlayer()->GetGuild();
    if (!guild)
    {
        recvPacket.rfinish();
        return;
    }

    uint32 oldRankId;
    uint32 newRankId;
    uint32 oldRights;
    uint32 newRights;
    uint32 moneyPerDay;

    recvPacket >> oldRankId;
    recvPacket >> oldRights;
    recvPacket >> newRights;

    GuildBankRightsAndSlotsVec rightsAndSlots(GUILD_BANK_MAX_TABS);
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        uint32 bankRights;
        uint32 slots;

        recvPacket >> bankRights;
        recvPacket >> slots;

        rightsAndSlots[tabId] = GuildBankRightsAndSlots(tabId, uint8(bankRights), slots);
    }

    recvPacket >> moneyPerDay;
    recvPacket >> newRankId;
    uint32 nameLength = recvPacket.ReadBits(7);
    std::string rankName = recvPacket.ReadString(nameLength);

    uint32 moneyDaily = moneyPerDay * GOLD; // Amount of gold which member can withdraw daily.

    guild->HandleSetRankInfo(this, newRankId, rankName, newRights, moneyDaily, rightsAndSlots);
}

void WorldSession::HandleGuildRequestPartyState(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_REQUEST_PARTY_STATE");

    ObjectGuid guildGuid;

    guildGuid[0] = recvData.ReadBit();
    guildGuid[6] = recvData.ReadBit();
    guildGuid[7] = recvData.ReadBit();
    guildGuid[3] = recvData.ReadBit();
    guildGuid[5] = recvData.ReadBit();
    guildGuid[1] = recvData.ReadBit();
    guildGuid[2] = recvData.ReadBit();
    guildGuid[4] = recvData.ReadBit();

    recvData.ReadByteSeq(guildGuid[6]);
    recvData.ReadByteSeq(guildGuid[3]);
    recvData.ReadByteSeq(guildGuid[2]);
    recvData.ReadByteSeq(guildGuid[1]);
    recvData.ReadByteSeq(guildGuid[5]);
    recvData.ReadByteSeq(guildGuid[0]);
    recvData.ReadByteSeq(guildGuid[7]);
    recvData.ReadByteSeq(guildGuid[4]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guildGuid))
        guild->HandleGuildPartyRequest(this);
}

void WorldSession::HandleAutoDeclineGuildInvites(WorldPacket& recvPacket)
{
    uint8 enable;
    recvPacket >> enable;

    GetPlayer()->ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_AUTO_DECLINE_GUILD, enable);
}

void WorldSession::HandleGuildRequestMaxDailyXP(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    guid[0] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[5] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[5]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[0]);

    if (Guild* guild = sGuildMgr->GetGuildByGuid(guid))
    {
        if (guild->IsMember(_player->GetGUID()))
        {
            WorldPacket data(SMSG_GUILD_MAX_DAILY_XP, 8);
            data << uint64(sWorld->getIntConfig(CONFIG_GUILD_DAILY_XP_CAP));
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleGuildRewardsQueryOpcode(WorldPacket& recvPacket)
{
    recvPacket.read_skip<uint32>(); // Unk

    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
    {
        std::vector<GuildReward> const& rewards = sGuildMgr->GetGuildRewards();

        WorldPacket data(SMSG_GUILD_REWARDS_LIST, 3 + rewards.size() * (4 + 4 + 4 + 8 + 4 + 4));
        data.WriteBits(rewards.size(), 21);
        data.FlushBits();

        for (uint32 i = 0; i < rewards.size(); i++)
        {
            data << uint32(rewards[i].Standing);
            data << int32(rewards[i].Racemask);
            data << uint32(rewards[i].Entry);
            data << uint64(rewards[i].Price);
            data << uint32(0); // Unused
            data << uint32(rewards[i].AchievementId);
        }
        data << uint32(time(NULL));
        SendPacket(&data);
    }
}

void WorldSession::HandleGuildQueryNewsOpcode(WorldPacket& recvPacket)
{
    recvPacket.read_skip<uint32>();

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->SendNewsUpdate(this);
}

void WorldSession::HandleGuildNewsUpdateStickyOpcode(WorldPacket& recvPacket)
{
    uint32 newsId;
    bool sticky;
    ObjectGuid guid;

    recvPacket >> newsId;

    guid[2] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    sticky = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[5] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[5]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[4]);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleNewsSetSticky(this, newsId, sticky);
}

void WorldSession::SendGuildCancelInvite(std::string unkString, uint8 unkByte)
{
    WorldPacket data(SMSG_GUILD_INVITE_CANCEL,1 + unkString.length());

    data << unkString << unkByte;

    SendPacket(&data);
}

void WorldSession::HandleGuildSetGuildMaster(WorldPacket& recvPacket)
{
    uint8 nameLength = recvPacket.ReadBits(7);
    // This is related to guild master inactivity.
    /*bool isDethrone = */recvPacket.ReadBit();
    std::string playerName = recvPacket.ReadString(nameLength);

    if (Guild* guild = GetPlayer()->GetGuild())
        guild->HandleSetNewGuildMaster(this, playerName);
}

void WorldSession::HandleGuildSwitchRank(WorldPacket& recvPacket)
{
    uint32 rank;
    bool direction;                 // if its true, then the rank rises, if no, it goes down

    recvPacket >> rank;
    direction = recvPacket.ReadBit();

    Guild* pGuild = GetPlayer()->GetGuild();

    if(pGuild)
    {
        pGuild->MoveRank(rank,direction);
    }
}

void WorldSession::HandleGuildAchievementMembers(WorldPacket& recvPacket)
{
    uint32 achievementId;
    ObjectGuid guildGuid;
    ObjectGuid playerGuid;

    recvPacket >> achievementId;

    guildGuid[0] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    playerGuid[3] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();
    playerGuid[2] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    playerGuid[6] = recvPacket.ReadBit();
    guildGuid[2] = recvPacket.ReadBit();
    playerGuid[1] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(playerGuid[2]);
    recvPacket.ReadByteSeq(guildGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[2]);
    recvPacket.ReadByteSeq(playerGuid[5]);
    recvPacket.ReadByteSeq(playerGuid[0]);
    recvPacket.ReadByteSeq(playerGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[4]);
    recvPacket.ReadByteSeq(playerGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[6]);
    recvPacket.ReadByteSeq(playerGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[4]);
    recvPacket.ReadByteSeq(guildGuid[6]);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Received CMSG_GUILD_ACHIEVEMENT_MEMBERS");

    if(Guild* pGuild = sGuildMgr->GetGuildByGuid(guildGuid))
    {
        if(pGuild->GetAchievementMgr().HasAchieved(achievementId))
        {
            ObjectGuid gguid = pGuild->GetGUID();
            CompletedAchievementData* achievement = pGuild->GetAchievementMgr().GetCompletedDataForAchievement(achievementId);
            WorldPacket data(SMSG_GUILD_ACHIEVEMENT_MEMBERS);

            data.WriteBit(gguid[3]);
            data.WriteBit(gguid[4]);
            data.WriteBit(gguid[7]);
            data.WriteBit(gguid[0]);

            data.WriteBits(achievement->guids.size(),26);

            for(std::set<uint64>::iterator itr = achievement->guids.begin(); itr != achievement->guids.end(); ++itr)
            {
                ObjectGuid pguid = (*itr);

                data.WriteBit(pguid[3]);
                data.WriteBit(pguid[1]);
                data.WriteBit(pguid[4]);
                data.WriteBit(pguid[5]);
                data.WriteBit(pguid[7]);
                data.WriteBit(pguid[0]);
                data.WriteBit(pguid[6]);
                data.WriteBit(pguid[2]);
            }

            data.WriteBit(gguid[2]);
            data.WriteBit(gguid[6]);
            data.WriteBit(gguid[5]);
            data.WriteBit(gguid[1]);

            data.FlushBits();

            data.WriteByteSeq(gguid[5]);

            for(std::set<uint64>::iterator itr = achievement->guids.begin(); itr != achievement->guids.end(); ++itr)
            {
                ObjectGuid pguid = (*itr);

                data.WriteByteSeq(pguid[1]);
                data.WriteByteSeq(pguid[5]);
                data.WriteByteSeq(pguid[7]);
                data.WriteByteSeq(pguid[0]);
                data.WriteByteSeq(pguid[6]);
                data.WriteByteSeq(pguid[4]);
                data.WriteByteSeq(pguid[3]);
                data.WriteByteSeq(pguid[2]);
            }

            data.WriteByteSeq(gguid[7]);
            data.WriteByteSeq(gguid[2]);
            data.WriteByteSeq(gguid[4]);
            data.WriteByteSeq(gguid[3]);
            data.WriteByteSeq(gguid[6]);
            data.WriteByteSeq(gguid[0]);

            data << achievementId;

            data.WriteByteSeq(gguid[1]);

            SendPacket(&data);
        }
    }
}

void WorldSession::HandleGuildRenameRequest(WorldPacket& recvPacket)
{
    uint32 lenght;
    std::string newName;

    lenght = recvPacket.ReadBits(8);
    newName = recvPacket.ReadString(lenght);

    Guild* pGuild = GetPlayer()->GetGuild();

    if(pGuild)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GUILD_NAME);

        _guildRenameCallback.SetParam(newName);

        stmt->setUInt32(1,pGuild->GetId());
        stmt->setString(0,newName);

        _guildRenameCallback.SetFutureResult(CharacterDatabase.AsyncQuery(stmt));

        WorldPacket data(SMSG_GUILD_FLAGGED_FOR_RENAME,1);

        data.WriteBit(true);            // it may send false also, but we don't know how to handle exeptions by the DB layer
        data.FlushBits();

        pGuild->BroadcastPacket(&data);
    }
}

void WorldSession::HandleGuildRenameCallback(std::string newName)
{
    Guild* pGuild = GetPlayer()->GetGuild();

    bool hasRenamed = ((PreparedQueryResult)_guildRenameCallback.GetFutureResult())->GetRowCount() > 0 ? true : false;

    WorldPacket data(SMSG_GUILD_CHANGE_NAME_RESULT,1);
    data.WriteBit(hasRenamed);
    data.FlushBits();

    SendPacket(&data);

    if(pGuild && hasRenamed)
    {
        pGuild->SendGuildRename(newName);
    }
}

void WorldSession::HandleGuildChallengeRequest(WorldPacket& recvPacket)
{
    uint8 counter = 4;

    if(Guild* pGuild = GetPlayer()->GetGuild())
    {
        if (Guild::ChallengesMgr* challengesMgr = pGuild->GetChallengesMgr())
        {
            WorldPacket data(SMSG_GUILD_CHALLENGE_UPDATED,5*4*4);

            //Guild Experience Reward block

            data << uint32(0);                                                                      //in the block its always 1
            data << challengesMgr->GetXPRewardForType(CHALLENGE_TYPE_DUNGEON);                      //dungeon
            data << challengesMgr->GetXPRewardForType(CHALLENGE_TYPE_RAID);                         //raid
            data << challengesMgr->GetXPRewardForType(CHALLENGE_TYPE_RATEDBG);                      //rated BG

            //Gold Bonus block
            data << uint32(0);                                                                      //in the block its always 1
            data << challengesMgr->GetGoldBonusForType(CHALLENGE_TYPE_DUNGEON);                     //dungeon
            data << challengesMgr->GetGoldBonusForType(CHALLENGE_TYPE_RAID);                        //raid
            data << challengesMgr->GetGoldBonusForType(CHALLENGE_TYPE_RATEDBG);                     //rated BG

            //Total Count block

            data << uint32(0);                                                                      //in the block its always 1
            data << challengesMgr->GetTotalCountFor(CHALLENGE_TYPE_DUNGEON);                        //dungeon
            data << challengesMgr->GetTotalCountFor(CHALLENGE_TYPE_RAID);                           //raid
            data << challengesMgr->GetTotalCountFor(CHALLENGE_TYPE_RATEDBG);                        //rated BG            

            //Completion Gold Reward block

            data << uint32(0);                                                                      //in the block its always 1
            data << challengesMgr->GetGoldRewardForType(CHALLENGE_TYPE_DUNGEON);                    //dungeon
            data << challengesMgr->GetGoldRewardForType(CHALLENGE_TYPE_RAID);                       //raid
            data << challengesMgr->GetGoldRewardForType(CHALLENGE_TYPE_RATEDBG);                    //rated BG

            //Current Count block

            data << uint32(0);                                                                      //in the block its always 1
            data << challengesMgr->GetCurrentCountFor(CHALLENGE_TYPE_DUNGEON);                      //dungeon
            data << challengesMgr->GetCurrentCountFor(CHALLENGE_TYPE_RAID);                         //raid
            data << challengesMgr->GetCurrentCountFor(CHALLENGE_TYPE_RATEDBG);                      //rated BG

            SendPacket(&data);
        }
        return;
    }
}