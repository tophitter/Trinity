#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Chat.h"
#include "Group.h"

class hassitem_commandscript : public CommandScript
{
public:
    hassitem_commandscript() : CommandScript("hassitem_commandscript") { }
	
 ChatCommand* GetCommands() const

   {
        static ChatCommand hassitemCommandTable[] =
        {
            { "item",            SEC_GAMEMASTER,  false, &HandlehassitemCommand,             "", NULL },
            { NULL,             0,                  false, NULL,                              "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "has",           SEC_GAMEMASTER,      true, NULL,                   "", hassitemCommandTable },
            { NULL,             0,                  false, NULL,                               "", NULL }
        };
        return commandTable;
    } 
    static bool HandlehassitemCommand(ChatHandler* handler, const char* args)
{
    if (!*args)
        return false;

    char* cId = extractKeyFromLink((char*)args, "Hitem");
    if (!cId)
        return false;

    uint32 item_id = atol(cId);
    if (!item_id)
    {
        PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, item_id);
        SetSentErrorMessage(true);
        return false;
    }

    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(item_id);
    if (!itemProto)
    {
        PSendSysMessage(LANG_COMMAND_ITEMIDINVALID, item_id);
        SetSentErrorMessage(true);
        return false;
    }

    char* c_count = strtok(NULL, " ");
    int _count = c_count ? atol(c_count) : 10;

    if (_count < 0)
        return false;
    uint32 count = uint32(_count);

    QueryResult result;

    // inventory case
    uint32 inv_count = 0;
    result = CharacterDatabase.PQuery("SELECT COUNT(itemEntry) FROM character_inventory ci INNER JOIN item_instance ii ON ii.guid = ci.item WHERE itemEntry = '%u'", item_id);
    if (result)
        inv_count = (*result)[0].GetUInt32();

    result=CharacterDatabase.PQuery(
    //          0        1               2        3        4          5
        "SELECT ci.item, cb.slot AS bag, ci.slot, ci.guid, c.account, c.name FROM characters c "
        "INNER JOIN character_inventory ci ON ci.guid = c.guid "
        "INNER JOIN item_instance ii ON ii.guid = ci.item "
        "LEFT JOIN character_inventory cb ON cb.item = ci.bag "
        "WHERE ii.itemEntry = '%u' LIMIT %u ",
        item_id, count);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid = fields[0].GetUInt32();
            uint32 item_bag = fields[1].GetUInt32();
            uint32 item_slot = fields[2].GetUInt32();
            uint32 owner_guid = fields[3].GetUInt32();
            uint32 owner_acc = fields[4].GetUInt32();
            std::string owner_name = fields[5].GetString();

            char const* item_pos = 0;
            if (Player::IsEquipmentPos(item_bag, item_slot))
                item_pos = "[equipped]";
            else if (Player::IsInventoryPos(item_bag, item_slot))
                item_pos = "[in inventory]";
            else if (Player::IsBankPos(item_bag, item_slot))
                item_pos = "[in bank]";
            else
                item_pos = "";

            PSendSysMessage(LANG_ITEMLIST_SLOT, item_guid, owner_name.c_str(), owner_guid, owner_acc, item_pos);
        }
        while (result->NextRow());

        uint32 res_count = uint32(result->GetRowCount());

        if (count > res_count)
            count -= res_count;
        else if (count)
            count = 0;
    }

    // mail case
    uint32 mail_count = 0;
    result = CharacterDatabase.PQuery("SELECT COUNT(itemEntry) FROM mail_items mi INNER JOIN item_instance ii ON ii.guid = mi.item_guid WHERE itemEntry = '%u'", item_id);
    if (result)
        mail_count = (*result)[0].GetUInt32();

    if (count > 0)
    {
        result = CharacterDatabase.PQuery(
        //          0             1         2           3           4        5           6
            "SELECT mi.item_guid, m.sender, m.receiver, cs.account, cs.name, cr.account, cr.name FROM mail m "
            "INNER JOIN mail_items mi ON mi.mail_id = m.id "
            "INNER JOIN item_instance ii ON ii.guid = mi.item_guid "
            "INNER JOIN characters cs ON cs.guid = m.sender "
            "INNER JOIN characters cr ON cr.guid = m.receiver "
            "WHERE ii.itemEntry = '%u' LIMIT %u",
            item_id, count);
    }
    else
        result = QueryResult(NULL);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid        = fields[0].GetUInt32();
            uint32 item_s           = fields[1].GetUInt32();
            uint32 item_r           = fields[2].GetUInt32();
            uint32 item_s_acc       = fields[3].GetUInt32();
            std::string item_s_name = fields[4].GetString();
            uint32 item_r_acc       = fields[5].GetUInt32();
            std::string item_r_name = fields[6].GetString();

            char const* item_pos = "[in mail]";

            PSendSysMessage(LANG_ITEMLIST_MAIL, item_guid, item_s_name.c_str(), item_s, item_s_acc, item_r_name.c_str(), item_r, item_r_acc, item_pos);
        }
        while (result->NextRow());

        uint32 res_count = uint32(result->GetRowCount());

        if (count > res_count)
            count -= res_count;
        else if (count)
            count = 0;
    }

    // auction case
    uint32 auc_count = 0;
    result=CharacterDatabase.PQuery("SELECT COUNT(itemEntry) FROM auctionhouse ah INNER JOIN item_instance ii ON ii.guid = ah.itemguid WHERE itemEntry = '%u'", item_id);
    if (result)
        auc_count = (*result)[0].GetUInt32();

    if (count > 0)
    {
        result = CharacterDatabase.PQuery(
        //           0            1             2          3
            "SELECT  ah.itemguid, ah.itemowner, c.account, c.name FROM auctionhouse ah "
            "INNER JOIN characters c ON c.guid = ah.itemowner "
            "INNER JOIN item_instance ii ON ii.guid = ah.itemguid "
            "WHERE ii.itemEntry = '%u' LIMIT %u", item_id, count);
    }
    else
        result = QueryResult(NULL);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid       = fields[0].GetUInt32();
            uint32 owner           = fields[1].GetUInt32();
            uint32 owner_acc       = fields[2].GetUInt32();
            std::string owner_name = fields[3].GetString();

            char const* item_pos = "[in auction]";

            PSendSysMessage(LANG_ITEMLIST_AUCTION, item_guid, owner_name.c_str(), owner, owner_acc, item_pos);
        }
        while (result->NextRow());
    }

    // guild bank case
    uint32 guild_count = 0;
    result = CharacterDatabase.PQuery("SELECT COUNT(itemEntry) FROM guild_bank_item gbi INNER JOIN item_instance ii ON ii.guid = gbi.item_guid WHERE itemEntry = '%u'", item_id);
    if (result)
        guild_count = (*result)[0].GetUInt32();

    result = CharacterDatabase.PQuery(
        //      0             1           2
        "SELECT gi.item_guid, gi.guildid, g.name FROM guild_bank_item gi "
        "INNER JOIN guild g ON g.guildid = gi.guildid "
        "INNER JOIN item_instance ii ON ii.guid = gi.item_guid "
        "WHERE ii.itemEntry = '%u' LIMIT %u ",
        item_id, count);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid = fields[0].GetUInt32();
            uint32 guild_guid = fields[1].GetUInt32();
            std::string guild_name = fields[2].GetString();

            char const* item_pos = "[in guild bank]";

            PSendSysMessage(LANG_ITEMLIST_GUILD, item_guid, guild_name.c_str(), guild_guid, item_pos);
        }
        while (result->NextRow());

        uint32 res_count = uint32(result->GetRowCount());

        if (count > res_count)
            count -= res_count;
        else if (count)
            count = 0;
    }

    if (inv_count + mail_count + auc_count + guild_count == 0)
    {
        SendSysMessage(LANG_COMMAND_NOITEMFOUND);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_COMMAND_LISTITEMMESSAGE, item_id, inv_count + mail_count + auc_count + guild_count, inv_count, mail_count, auc_count, guild_count);
    return true;
}
};

void AddSC_hassitem_commandscript()
{
    new hassitem_commandscript();
}



	