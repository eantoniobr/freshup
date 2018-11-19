#pragma once

#include <string>
#include <vector>
#include <Poco/LocalDateTime.h>
#include <Poco/DateTime.h>
#include "itemdb.h"

#include "../common/typedef.h"

#define MAX_AMOUNT_ITEM 30000
#define pc_item_isrent(flag) ( (flag == ITEMDB_PERIOD) || (flag == ITEMDB_SKIN_PERIOD) || (flag == ITEMDB_PART_RENT) )

class pc;
class Packet;
struct Item;
struct INV_TRANSACTION;

typedef std::shared_ptr<Item> PC_ITEM;
typedef std::shared_ptr<INV_TRANSACTION> ITEM_TRANSACTION;

enum e_checkitem_result {
	CHECKITEM_PASS,
	CHECKITEM_FAIL,
	CHECKITEM_OVERLIMIT,
	CHECKITEM_DUPLICATED
};

enum e_additem_result {
	ADDITEM_SUCCESS,
	ADDITEM_DUPLICATED,
	ADDITEM_INVALID,
	ADDITEM_OVERWEIGHT,
	ADDITEM_OVERITEM,
	ADDITEM_OVERAMOUNT,
	ADDITEM_STACKLIMIT
};

enum {
	DELITEM_SUCCESS,
	DELITEM_AMOUNT_NOTENOUGHT,
	DELITEM_ITEM_NOTFOUND,
	DELITEM_ERROR
};

enum inventory_type {
	IV_CHAR,
	IV_ALLITEM,
	IV_CADDIE,
	IV_CARD,
	IV_EQUIPMENT
};

enum find_by {
	FIND_BY_ID,
	FIND_BY_TYPEID
};

struct item {
	uint8 item_type = 2;
	uint32 type_id;
	uint16 amount;
	uint16 day_amount = 0;
	uint8 flag;
	std::string ucc_string = "";
};

struct PC_Equipment {
	uint32 caddie_id;
	uint32 char_id;
	uint32 club_id;
	uint32 ball_id;
	uint32 item_slot[10];
	uint32 title_id;
	uint32 mascot_id;
	uint32 poster1;
	uint32 poster2;
};

/* New implement */

struct Item {
	uint32 id;
	uint32 item_typeid;
	uint16 c0, c1, c2, c3, c4 = 0;
	Poco::DateTime create_date;
	Poco::DateTime end_date;
	uint8 flag;
	uint8 type;

	std::string ucc_string = "";
	std::string ucc_key = "";
	uint8 ucc_state = 0;
	uint32 ucc_copy_count = 0;
	std::string ucc_drawer = "";

	uint8 hair_colour = 0; // character

	std::string message = ""; // mascot

	uint32 equip_typeid[24];
	uint32 equip_index[24];

	bool valid = true;
	bool sync = false;

	Item(int init_id = 0, int init_typeid = 0, int init_amount = 0, int init_hair_colour = 0) {
		id = init_id;
		item_typeid = init_typeid;
		c0 = init_amount;
		hair_colour = init_hair_colour;
	}
};

struct INV_TRANSACTION {
	uint8 type = 2;
	uint32 item_id;
	uint32 item_typeid;
	uint32 old_amount = 0;
	uint32 new_amount = 0;
	uint32 timestamp_reg = 0;
	uint32 timestamp_end = 0;
	std::string ucc_unique = "";
	uint8 ucc_status = 0;
	uint8 ucc_copycount = 0;
	uint16 c0, c1, c2, c3, c4 = 0;
	uint32 club_point = 0;
	uint32 club_count = 0;
	uint32 club_cancelcount = 0;
	uint32 card_typeid = 0;
	uint8 char_slot = 0;

	INV_TRANSACTION(PC_ITEM const& item, int in_old_amount = 0) {
		item_id = item->id;
		item_typeid = item->item_typeid;
		old_amount = in_old_amount;
		new_amount = item->c0;
		ucc_unique = item->ucc_string;
		ucc_status = item->ucc_state;
		ucc_copycount = item->ucc_copy_count;
		c0 = item->c0;
		c1 = item->c1;
		c2 = item->c2;
		c3 = item->c3;
		c4 = item->c4;
	}

	INV_TRANSACTION() {};
};

struct Club_Data {
	uint32 item_id = 0;
	uint8 c0, c1, c2, c3, c4 = 0;
	uint32 point = 0;
	uint32 work_count = 0;
	uint32 cancel_count = 0;
	uint32 point_total = 0;
	uint32 pang_total = 0;
};

struct PC_Warehouse {
private:
	std::vector<PC_ITEM> inventory;
	std::vector<std::shared_ptr<Club_Data>> club_data_;
	std::shared_ptr<PC_Equipment> equipment;

	void put_transaction(PC_ITEM const& item);
	int item_count(inventory_type type_name);
	uint16 get_time_left(PC_ITEM const& item); // as hour
public:
	void load_data(pc* pc);
	void send_data(pc* pc, inventory_type type_name);
	void write_current_char(Packet* p);
	uint32 char_typeid_equiped();

	char additem(pc* pc, item* item, bool test_additem = false, ITEM_TRANSACTION* tran = nullptr);
	char delitem(pc* pc, int item_typeid, int amount, ITEM_TRANSACTION* tran = nullptr);

	void savedata(pc *pc);

	PC_Warehouse();
};