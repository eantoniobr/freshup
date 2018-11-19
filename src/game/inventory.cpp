#include "inventory.h"
#include "itemdb.h"
#include "pc.h"

#include "../common/packet.h"
#include "../common/db.h"
#include "../common/utils.h"

#include <boost/format.hpp>

#include "spdlog/sinks/stdout_color_sinks.h"

PC_Warehouse::PC_Warehouse() :
	equipment(CREATE_SHARED(PC_Equipment)) {}

void PC_Warehouse::load_data(pc* pc) {
	/* Load char data from sql */
	{
		Statement stm(*get_session());
		stm << "SELECT * FROM char WHERE account_id = ?", use(pc->account_id_), now;
		RecordSet rs(stm);

		if (rs.rowCount() <= 0) {
			pc->disconnect();
			return;
		}

		bool done = rs.moveFirst();

		while (done) {
			std::shared_ptr<Item> item = CREATE_SHARED(Item);
			item->id = rs["char_id"];
			item->item_typeid = rs["char_typeid"];
			item->hair_colour = rs["hair_color"];
			item->c0 = rs["c0"];
			item->c1 = rs["c1"];
			item->c2 = rs["c2"];
			item->c3 = rs["c3"];
			item->c4 = rs["c4"];
			item->flag = rs["flag"];
			inventory.push_back(item);
			done = rs.moveNext();
		}
	}

	/* load char equipment */
	{
		Statement stm(*get_session());
		stm << "SELECT * FROM char_equip WHERE account_id = ?", use(pc->account_id_), now;
		RecordSet rs(stm);

		bool done = rs.moveFirst();

		while (done) {
			uint32 char_id = rs["char_id"];
			uint32 item_id = rs["item_id"];
			uint32 item_typeid = rs["item_typeid"];
			uint8 item_num = rs["num"];

			auto find_char = std::find_if(inventory.begin(), inventory.end(), [&char_id](PC_ITEM const& ritem) {
				return ritem->id == char_id;
			});

			if (find_char != inventory.end()) {
				(*find_char)->equip_index[item_num - 1] = item_id;
				(*find_char)->equip_typeid[item_num - 1] = item_typeid;
			}

			done = rs.moveNext();
		}
	}

	/* Load item data from sql */
	{
		Statement stm(*get_session());
		stm << "SELECT * FROM inventory WHERE account_id = ? AND valid = 1", use(pc->account_id_), now;
		RecordSet rs(stm);

		bool done = rs.moveFirst();

		while (done) {
			std::shared_ptr<Item> item = CREATE_SHARED(Item);
			item->id = rs["id"];
			item->item_typeid = rs["typeid"];
			item->c0 = rs["c0"];
			item->c1 = rs["c1"];
			item->c2 = rs["c2"];
			item->c3 = rs["c3"];
			item->c4 = rs["c4"];
			item->flag = rs["flag"];
			item->type = rs["item_type"];
			item->create_date = (Poco::DateTime)rs["reg_date"];
			item->end_date = (Poco::DateTime)rs["end_date"];
			inventory.push_back(item);
			done = rs.moveNext();
		}
	}

	/* Load ClubSet data from sql */
	{
		Statement stm(*get_session());
		stm << "SELECT A.* FROM club_data A "
			<< "INNER JOIN inventory B ON B.id = A.item_id AND B.account_id = ? AND B.valid = 1", use(pc->account_id_), now;

		RecordSet rs(stm);

		bool done = rs.moveFirst();

		while (done) {
			std::shared_ptr<Club_Data> club = CREATE_SHARED(Club_Data);
			club->item_id = rs["item_id"];
			club->c0 = rs["c0"];
			club->c1 = rs["c1"];
			club->c2 = rs["c2"];
			club->c3 = rs["c3"];
			club->c4 = rs["c4"];
			club->point = rs["point"];
			club->work_count = rs["work_count"];
			club->cancel_count = rs["cancel_count"];
			club->point_total = rs["point_total"];
			club->pang_total = rs["pang_total"];
			club_data_.push_back(club);
			done = rs.moveNext();
		}
	}

	/* Load card from sql */
	{
		Statement stm(*get_session());
		stm << "SELECT * FROM card WHERE account_id = ? AND valid = 1", use(pc->account_id_), now;
		RecordSet rs(stm);

		bool done = rs.moveFirst();
		while (done) {
			std::shared_ptr<Item> item = CREATE_SHARED(Item);
			item->id = rs["id"];
			item->item_typeid = rs["typeid"];
			item->c0 = rs["amount"];
			inventory.push_back(item);
			done = rs.moveNext();
		}
	}

	/* Load pc equipment */
	{
		Statement stm(*get_session());
		stm << "SELECT * FROM equipment WHERE account_id = ?", use(pc->account_id_), now;

		RecordSet rs(stm);

		if (rs.rowCount() > 0) {
			equipment->caddie_id = rs["caddie_id"];
			equipment->club_id = rs["club_id"];
			equipment->char_id = rs["character_id"];
			equipment->ball_id = rs["ball_typeid"];
			equipment->item_slot[0] = rs["item_slot_1"];
			equipment->item_slot[1] = rs["item_slot_2"];
			equipment->item_slot[2] = rs["item_slot_3"];
			equipment->item_slot[3] = rs["item_slot_4"];
			equipment->item_slot[4] = rs["item_slot_5"];
			equipment->item_slot[5] = rs["item_slot_6"];
			equipment->item_slot[6] = rs["item_slot_7"];
			equipment->item_slot[7] = rs["item_slot_8"];
			equipment->item_slot[8] = rs["item_slot_9"];
			equipment->item_slot[9] = rs["item_slot_10"];
			equipment->mascot_id = rs["mascot_id"];
		}
	}
}

int PC_Warehouse::item_count(inventory_type type_name) {
	int count = 0;

	switch (type_name) {
	case IV_CHAR:
		for (auto &item : inventory) {
			if (itemdb_type(item->item_typeid) == ITEMDB_CHAR) {
				count += 1;
			}
		}
		break;
	case IV_ALLITEM:
		for (auto &item : inventory) {
			uint8 item_type = itemdb_type(item->item_typeid);
			if (item_type == ITEMDB_PART
				|| item_type == ITEMDB_CLUB
				|| item_type == ITEMDB_BALL
				|| item_type == ITEMDB_USE
				|| item_type == ITEMDB_SKIN
				|| item_type == ITEMDB_AUX)
			{
				count += 1;
			}
		}
		break;
	case IV_CARD:
		for (auto &item : inventory) {
			if (itemdb_type(item->item_typeid) == ITEMDB_CARD) {
				count += 1;
			}
		}
		break;
	}

	return count;
}

uint16 PC_Warehouse::get_time_left(PC_ITEM const& item) {
	if (pc_item_isrent(item->flag)) {
		return static_cast<uint32>( ( TIMESTAMP(item->end_date) - TIMESTAMP(item->create_date) ) / 3600 );
	}
	return 0;
}

void PC_Warehouse::send_data(pc* pc, inventory_type type_name) {

	Packet p;
	int count = 0;

	switch (type_name) {
	case IV_CHAR:
	/* New Scope */
	{
		count = item_count(IV_CHAR);
		WTHEAD(&p, 0x70);
		WTIU16(&p, count);
		WTIU16(&p, count);

		for (auto &item : inventory) {
			if (itemdb_type(item->item_typeid) == ITEMDB_CHAR) {
				WTIU32(&p, item->item_typeid);
				WTIU32(&p, item->id);
				WTIU16(&p, item->hair_colour);
				WTIU16(&p, item->flag);

				for (int i = 0; i < 24; ++i) {
					WTIU32(&p, item->equip_typeid[i]);
				}
				
				for (int i = 0; i < 24; ++i) {
					WTIU32(&p, item->equip_index[i]);
				}

				WTZERO(&p, 0xD8);
				WTIU32(&p, 0); // LEFT RING
				WTIU32(&p, 0); // RIGHT RING
				WTZERO(&p, 12); // ?
				WTIU32(&p, 0); // CUTIN INDEX
				WTZERO(&p, 12);
				WTIU08(&p, (uint8)item->c0);
				WTIU08(&p, (uint8)item->c1);
				WTIU08(&p, (uint8)item->c2);
				WTIU08(&p, (uint8)item->c3);
				WTIU08(&p, (uint8)item->c4);
				WTIU08(&p, 0); // MASTERY POINT
				WTZERO(&p, 3);
				WTZERO(&p, 40); // CARD DATA
				WTIU32(&p, 0);
				WTIU32(&p, 0);
			}
		}
	}
	/* End Scope */
	break;
	case IV_ALLITEM:
	/* New Scope */
	{
		count = item_count(IV_ALLITEM);
		WTHEAD(&p, 0x73);
		WTIU16(&p, count);
		WTIU16(&p, count);

		for (auto &item : inventory) {
			uint8 item_type = itemdb_type(item->item_typeid);
			if (item_type == ITEMDB_PART
				|| item_type == ITEMDB_CLUB
				|| item_type == ITEMDB_BALL
				|| item_type == ITEMDB_USE
				|| item_type == ITEMDB_SKIN
				|| item_type == ITEMDB_AUX)
			{
				WTIU32(&p, item->id);
				WTIU32(&p, item->item_typeid);
				WTIU32(&p, get_time_left(item));
				WTIU16(&p, item->c0);
				WTIU16(&p, item->c1);
				WTIU16(&p, item->c2);
				WTIU16(&p, item->c3);
				WTIU16(&p, item->c4);
				WTZERO(&p, 1);
				WTIU08(&p, item->flag);

				WTIU32(&p, pc_item_isrent(item->flag) ? (uint32)TIMESTAMP(item->create_date) : 0);
				WTIU32(&p, 0);
				WTIU32(&p, pc_item_isrent(item->flag) ? (uint32)TIMESTAMP(item->end_date) : 0);
				WTZERO(&p, 4);
				WTIU08(&p, 2);
				WTFSTR(&p, "", 16); // UCC NAME
				WTZERO(&p, 25);
				WTFSTR(&p, "", 9); // UCC UNIQUE
				WTIU08(&p, 0); // UCC STATUS
				WTIU16(&p, 0); // UCC COPY COUNT
				WTFSTR(&p, "", 16); // UCC DRAWER NAME
				WTZERO(&p, 60);

				if (item_type == ITEMDB_CLUB) {
					// club status
					auto club_data = std::find_if(club_data_.begin(), club_data_.end(), [&item](std::shared_ptr<Club_Data> const& rcd) {
						return rcd->item_id == item->id;
					});

					bool valid = club_data != club_data_.end();
					WTIU16(&p, valid ? (*club_data)->c0 : 0);
					WTIU16(&p, valid ? (*club_data)->c1 : 0);
					WTIU16(&p, valid ? (*club_data)->c2 : 0);
					WTIU16(&p, valid ? (*club_data)->c3 : 0);
					WTIU16(&p, valid ? (*club_data)->c4 : 0);
					WTIU32(&p, valid ? (*club_data)->point : 0);
					WTIU32(&p, valid ? (*club_data)->cancel_count : 0);
					WTIU32(&p, valid ? (*club_data)->work_count : 0);
				}
				else {
					WTZERO(&p, 22);
				}

				WTIU32(&p, 0);
			}
		}
	}
	/* End Scope */
	break;
	case IV_CARD:
	/* New Scope */
	{
		count = item_count(IV_CARD);
		WTHEAD(&p, 0x138);
		WTIU32(&p, 0);
		WTIU16(&p, count);

		for (auto &item : inventory) {
			if (itemdb_type(item->item_typeid) == ITEMDB_CARD) {
				WTIU32(&p, item->id);
				WTIU32(&p, item->item_typeid);
				WTZERO(&p, 12);
				WTIU32(&p, item->c0);
				WTZERO(&p, 0x20);
				WTIU16(&p, 1);
			}
		}
	}
	/* End Scope */
	break;
	case IV_EQUIPMENT:
	/* New Scope */
	{
		WTHEAD(&p, 0x72);
		WTIU32(&p, equipment->caddie_id);
		WTIU32(&p, equipment->char_id);
		WTIU32(&p, equipment->club_id);
		WTIU32(&p, equipment->ball_id);
		WTIU32(&p, equipment->item_slot[0]);
		WTIU32(&p, equipment->item_slot[1]);
		WTIU32(&p, equipment->item_slot[2]);
		WTIU32(&p, equipment->item_slot[3]);
		WTIU32(&p, equipment->item_slot[4]);
		WTIU32(&p, equipment->item_slot[5]);
		WTIU32(&p, equipment->item_slot[6]);
		WTIU32(&p, equipment->item_slot[7]);
		WTIU32(&p, equipment->item_slot[8]);
		WTIU32(&p, equipment->item_slot[9]);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0); // TITLE INDEX
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0);
		WTIU32(&p, 0); // TITLE TYPEID
		WTIU32(&p, equipment->mascot_id);
		WTIU32(&p, 0); // POSTER 1
		WTIU32(&p, 0); // POSTER 2
	}
	/* End Scope */
	break;
	}

	pc->send_packet(&p);
}

char PC_Warehouse::additem(pc* pc, item* item, bool test_additem, ITEM_TRANSACTION* tran) {
	assert(pc);
	assert(item);

	auto item_find = std::find_if(inventory.begin(), inventory.end(), [&item](PC_ITEM const& ritem) {
		return ritem->item_typeid == item->type_id && ritem->valid == 1;
	});

	// declare sql variable
	Statement stm(*get_session());

	int old_amount = 0;

	switch (itemdb_type(item->type_id)) {
	case ITEMDB_CHAR:
	{
		if (item_find != inventory.end()) return ADDITEM_DUPLICATED;
		if (test_additem) return ADDITEM_SUCCESS;
	}
	break;
	case ITEMDB_CARD:
	{
		// if the item can be consumable and valid
		if (item_find != inventory.end()) {
			if (test_additem) return ADDITEM_SUCCESS;
			old_amount = (*item_find)->c0;
			(*item_find)->c0 += item->amount;
			(*item_find)->sync = true;
			if (tran)
				tran->reset(new INV_TRANSACTION(*item_find, old_amount));
			return ADDITEM_SUCCESS;
		}
		if (test_additem) return ADDITEM_SUCCESS;
	}
	break;
	case ITEMDB_USE:
	{
		// still have items
		if (item_find != inventory.end()) {
			if ((*item_find)->c0 + item->amount >= MAX_AMOUNT_ITEM) {
				return ADDITEM_STACKLIMIT;
			}
			if (test_additem) return ADDITEM_SUCCESS;
			old_amount = (*item_find)->c0;
			(*item_find)->c0 += item->amount;
			(*item_find)->sync = true;
			if (tran)
				tran->reset(new INV_TRANSACTION(*item_find, old_amount));
			return ADDITEM_SUCCESS;
		}
		if (item->amount > MAX_AMOUNT_ITEM) {
			return ADDITEM_STACKLIMIT;
		}
		if (test_additem) return ADDITEM_SUCCESS;
	}
	break;
	default:
		throw ItemTypeNotFound();
		return ADDITEM_INVALID;
		break;
	}

	stm << "EXEC sys_add_item ?, ?, ?", use(pc->account_id_), use(item->type_id), use(item->amount), now;
	RecordSet rs(stm);

	if (rs["item_id"] == 0) {
		return ADDITEM_INVALID;
	}

	PC_ITEM new_item = std::make_shared<Item>();
	new_item->item_typeid = rs["item_typeid"];
	new_item->id = rs["item_id"];
	new_item->c0 = rs["c0"];
	new_item->c1 = rs["c1"];
	new_item->c2 = rs["c2"];
	new_item->c3 = rs["c3"];
	new_item->c4 = rs["c4"];
	new_item->create_date = (Poco::DateTime)rs["create_date"];
	new_item->end_date = (Poco::DateTime)rs["end_date"];
	new_item->type = rs["type"];
	new_item->flag = rs["flag"];
	new_item->ucc_string = rs["ucc_string"].toString();
	new_item->ucc_key = rs["ucc_key"].toString();
	new_item->ucc_state = rs["ucc_state"];
	new_item->ucc_copy_count = rs["ucc_copy_count"];
	new_item->ucc_drawer = rs["ucc_drawer"].toString();
	new_item->hair_colour = rs["hair_colour"];

	inventory.push_back(new_item);

	if (tran)
		tran->reset(new INV_TRANSACTION(new_item, 0));
	return ADDITEM_SUCCESS;
}

char PC_Warehouse::delitem(pc* pc, int item_typeid, int amount, ITEM_TRANSACTION* tran) {
	assert(pc);

	auto item = std::find_if(inventory.begin(), inventory.end(), [&item_typeid](PC_ITEM const& ritem) {
		return ritem->item_typeid == item_typeid && ritem->valid == 1;
	});

	int old_amount = 0;

	switch (itemdb_type(item_typeid)) {
	case ITEMDB_CARD:
	{
		// IF ITEM NOT FOUND
		if (!VECTOR_FINDIF(inventory, item))
			return DELITEM_ITEM_NOTFOUND;

		// IF NOT ENOUGHT AMOUNT
		if ((*item)->c0 < amount)
			return DELITEM_AMOUNT_NOTENOUGHT;

		old_amount = (*item)->c0;
		(*item)->c0 -= amount;
		(*item)->sync = true;

		if ((*item)->c0 <= 0)
			(*item)->valid = false;

		if (tran)
			tran->reset(new INV_TRANSACTION((*item), old_amount));

		return DELITEM_SUCCESS;
	}
	break;
	}

	return DELITEM_ERROR;
}


void PC_Warehouse::put_transaction(PC_ITEM const& item) {

}

uint32 PC_Warehouse::char_typeid_equiped() {
	uint32 char_id = equipment->char_id;

	auto chars = std::find_if(inventory.begin(), inventory.end(), [&char_id](PC_ITEM const& item) {
		return item->id == char_id;
	});

	if (VECTOR_FINDIF(inventory, chars)) {
		return (*chars)->item_typeid;
	}

	return 0;
}

void PC_Warehouse::write_current_char(Packet* p) {
	uint32 char_id = equipment->char_id;

	auto chars = std::find_if(inventory.begin(), inventory.end(), [&char_id](PC_ITEM const& ritem) {
		return ritem->id == char_id;
	});

	if ( !VECTOR_FINDIF(inventory, chars) ) throw "Error, cannot get char data!";

	WTIU32(p, (*chars)->item_typeid);
	WTIU32(p, (*chars)->id);
	WTIU16(p, (*chars)->hair_colour);
	WTIU16(p, (*chars)->flag);

	for (int i = 0; i < 24; ++i) {
		WTIU32(p, (*chars)->equip_typeid[i]);
	}

	for (int i = 0; i < 24; ++i) {
		WTIU32(p, (*chars)->equip_index[i]);
	}

	WTZERO(p, 0xD8);
	WTIU32(p, 0); // LEFT RING
	WTIU32(p, 0); // RIGHT RING

	WTIU32(p, 0);
	WTIU32(p, 0);
	WTIU32(p, 0);

	WTIU32(p, 0); // CUTIN INDEX

	WTIU32(p, 0);
	WTIU32(p, 0);
	WTIU32(p, 0);

	WTIU08(p, (uint8)(*chars)->c0);
	WTIU08(p, (uint8)(*chars)->c1);
	WTIU08(p, (uint8)(*chars)->c2);
	WTIU08(p, (uint8)(*chars)->c3);
	WTIU08(p, (uint8)(*chars)->c4);
	WTIU08(p, 0); // MASTERY POINT

	WTZERO(p, 3);

	WTZERO(p, 40); // CARD DATA

	WTIU32(p, 0);
	WTIU32(p, 0);
}

void PC_Warehouse::savedata(pc *pc) {

	Poco::Timestamp now;

	int updateCount = 0;

	Statement stm(*get_session());

	std::string sql_string;

	// CARD
	{
		boost::format sql_format("UPDATE card SET amount = %1%, valid = %2% WHERE account_id = %3% AND id = %4%;");

		for (auto &it : inventory) {
			if (itemdb_type(it->item_typeid) == ITEMDB_CARD && it->sync) {
				sql_string.append( (sql_format % it->c0 % it->valid % pc->account_id_ % it->id).str() );
				updateCount += 1;
			}
		}

		stm << sql_string;

		if (updateCount > 0)
			stm.execute();
	}

	Poco::Timestamp::TimeDiff diff = now.elapsed() / 1000;
	spdlog::get("console")->info("PC saved data takes {} = {}ms", now.elapsed(), diff);
}