//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "definitions.h"

#include "ioban.h"
#include "iologindata.h"
#include "tools.h"
#include "database.h"

bool IOBan::isIpBanished(uint32_t ip, uint32_t mask /*= 0xFFFFFFFF*/)
{
	if(ip == 0)
		return false;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	/*query << "SELECT `expires` FROM `bans` WHERE ((" << ip << " & " << mask << " & `param`) = (`value` & `param` & " << mask << ")) AND `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t expires = result->getDataLong("expires");
	db->freeResult(result);
	if(expires == 0 || time(NULL) <= (time_t)expires)
		return true;

	removeIpBanishment(ip);
	return false;*/

	query << "SELECT `value`, `param`, `expires` FROM `bans` WHERE `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	bool ret = false;
	do
	{
		if((ip & mask & result->getDataInt("param")) == (result->getDataInt("value") & result->getDataInt("param") & mask))
		{
			if(result->getDataLong("expires") == 0 || time(NULL) <= (time_t)result->getDataLong("expires"))
				ret = true;
			else
				removeIpBanishment(ip);
		}
	}
	while(result->next());

	db->freeResult(result);
	return ret;
}

bool IOBan::isNamelocked(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << guid << " AND `type` = " << (BanType_t)BANTYPE_NAMELOCK << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOBan::isNamelocked(std::string name)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return isNamelocked(_guid);
}

bool IOBan::isBanished(uint32_t account)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t expires = result->getDataInt("expires");
	db->freeResult(result);
	if(expires == 0 || time(NULL) <= (time_t)expires)
		return true;

	removeBanishment(account);
	return false;
}

bool IOBan::isDeleted(uint32_t account)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOBan::addIpBanishment(uint32_t ip, time_t banTime, std::string comment, uint32_t gamemaster)
{
	if(isIpBanished(ip))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`) VALUES (" << (BanType_t)BANTYPE_IP_BANISHMENT << ", " << ip << ", 4294967295, " << banTime << ", " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ")";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addNamelock(uint32_t playerId, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	if(isNamelocked(playerId))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_NAMELOCK << ", " << playerId << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addNamelock(std::string name, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return addNamelock(_guid, reasonId, actionId, comment, gamemaster);
}

bool IOBan::addBanishment(uint32_t account, time_t banTime, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	if(isBanished(account) || isDeleted(account))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_BANISHMENT << ", " << account << ", " << banTime << ", " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addDeletion(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	if(isDeleted(account))
		return false;

	if(!removeBanishment(account))
	{
		std::cout << "[Error - IOBan::addDeletion]: Couldn't remove banishments" << std::endl;
		return false;
	}

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_DELETION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

void IOBan::addNotation(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_NOTATION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
}

bool IOBan::removeIpBanishment(uint32_t ip)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << ip << " AND `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeNamelock(uint32_t guid)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << guid << " AND `type` = " << (BanType_t)BANTYPE_NAMELOCK;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeNamelock(std::string name)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return removeNamelock(_guid);
}

bool IOBan::removeBanishment(uint32_t account)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeDeletion(uint32_t account)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

void IOBan::removeNotations(uint32_t account)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION;
	db->executeQuery(query.str());
}

uint32_t IOBan::getReason(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `reason` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("reason");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getAction(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `action` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("action");
	db->freeResult(result);
	return value;
}

uint64_t IOBan::getExpireTime(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint64_t value = result->getDataInt("expires");
	db->freeResult(result);
	return value;
}

uint64_t IOBan::getAddedTime(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `added` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint64_t value = result->getDataInt("added");
	db->freeResult(result);
	return value;
}

std::string IOBan::getComment(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `comment` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return "";

	const std::string value = result->getDataString("comment");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getAdminGUID(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `admin_id` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("admin_id");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getNotationsCount(uint32_t account)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT COUNT(`id`) AS `count` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t count = result->getDataInt("count");
	db->freeResult(result);
	return count;
}

bool IOBan::getData(uint32_t account, Ban& ban)
{
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t currentTime = time(NULL);

	DBQuery query;
	query << "SELECT `id`, `type`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action` FROM `bans` WHERE `value` = " << account << " AND `active` = 1 AND (`type` = 3 OR `type` = 5) AND (`expires` > " << currentTime << " OR `expires` <= 0)";

	if(!(result = db->storeQuery(query.str())))
		return false;

	ban.value = account;
	ban.id = result->getDataInt("id");
	ban.type = (BanType_t)result->getDataInt("type");
	ban.param = result->getDataString("param");
	ban.expires = (uint32_t)result->getDataLong("expires");
	ban.added = (uint32_t)result->getDataLong("added");
	ban.adminid = result->getDataInt("admin_id");
	ban.comment = result->getDataString("comment");
	ban.reason = result->getDataInt("reason");
	ban.action = result->getDataInt("action");

	db->freeResult(result);
	return true;
}

BansVec IOBan::getList(BanType_t type)
{
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t currentTime = time(NULL);

	DBQuery query;
	query << "SELECT `id`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action` FROM `bans` WHERE `type` = " << type << " AND `active` = 1 AND (`expires` > " << currentTime << " OR `expires` <= 0)";

	BansVec data;
	if((result = db->storeQuery(query.str())))
	{
		Ban tmp;
		do {
			tmp.type = type;
			tmp.id = result->getDataInt("id");
			tmp.value = result->getDataString("value");
			tmp.param = result->getDataString("param");
			tmp.expires = (uint32_t)result->getDataLong("expires");
			tmp.added = (uint32_t)result->getDataLong("added");
			tmp.adminid = result->getDataInt("admin_id");
			tmp.comment = result->getDataString("comment");
			tmp.reason = result->getDataInt("reason");
			tmp.action = result->getDataInt("action");
			data.push_back(tmp);
		}
		while(result->next());
		db->freeResult(result);
	}

	return data;
}

bool IOBan::clearTemporials()
{
	Database* db = Database::getInstance();
	return db->executeQuery("UPDATE `bans` SET `active` = 0 WHERE `expires` = 0 AND `active` = 1;");
}
