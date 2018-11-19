#include "itemdb.h"

#include "clif.h"
#include "../common/packet.h"
#include "../common/utils.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <fstream>
#include <sstream> // istringstream
#include <iostream> // DELETE SOON

#include "pc.h" // PC CLASS

ItemDB* itemdb = nullptr;

ItemDB::ItemDB() {
	readdb_normal();
	readdb_part();
	readdb_club();
	readdb_ball();
	readdb_card();
	readdb_magicbox();
	read_random_group("config/random.txt");
}

void ItemDB::readdb_normal() {
	std::fstream f;
	int count;

	f.open("db/Item.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(4, std::ios::cur);
	
	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<itemdb_normal> it = CREATE_SHARED(itemdb_normal);
		f.read((char*)&it->base.enable, sizeof(itemdb_normal));
		item_data[it->base.item_typeid] = it;
	}
	f.close();
}

void ItemDB::readdb_part() {
	std::fstream f;
	int count;

	f.open("db/Part.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(4, std::ios::cur);

	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<itemdb_part> it = CREATE_SHARED(itemdb_part);
		f.read((char*)&it->base.enable, sizeof(itemdb_part));
		part_data[it->base.item_typeid] = it;
	}
	f.close();
}

void ItemDB::readdb_club() {
	std::fstream f;
	int count;

	f.open("db/ClubSet.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(4, std::ios::cur);

	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<itemdb_club> it = CREATE_SHARED(itemdb_club);
		f.read((char*)&it->base.enable, sizeof(itemdb_club));
		club_data[it->base.item_typeid] = it;
	}

	f.close();
}

void ItemDB::readdb_ball() {
	std::fstream f;
	int count;

	f.open("db/Ball.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(4, std::ios::cur);

	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<itemdb_ball> it = CREATE_SHARED(itemdb_ball);
		f.read((char*)&it->base.enable, sizeof(itemdb_ball));
		ball_data[it->base.item_typeid] = it;
	}
	f.close();
}

void ItemDB::readdb_card() {
	std::fstream f;

	std::ofstream myfile;
	myfile.open("example.txt");

	int count;

	f.open("db/Card.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(4, std::ios::cur);

	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<itemdb_card> it = CREATE_SHARED(itemdb_card);
		f.read((char*)&it->base.enable, sizeof(itemdb_card));
		if (strlen(it->base.name) <= 0) continue;

		if (it->in_vol == 1) {

			int rate = 0;

			if (it->type == 0) {
				rate = 50;
			}
			else if (it->type == 1) {
				rate = 30;
			}
			else if (it->type == 2) {
				rate = 10;
			}
			else if (it->type == 3) {
				rate = 5;
			}

			myfile << 2092957696 << "," << it->base.item_typeid << "," << rate << "," << 1 <<  "," << 1 << " //" << it->base.name << " x1"  << "\n";
		}

		//printf("%d, %s\n" , it->base.item_typeid, it->base.name);
		card_data[it->base.item_typeid] = it;
	}

	f.close();
	myfile.close();
}

void ItemDB::readdb_magicbox() {
	std::fstream f;
	uint16 count;

	f.open("db/CadieMagicBox.iff", std::fstream::in | std::fstream::binary);
	f.read((char*)&count, sizeof(count));
	f.seekg(6, std::ios::cur);

	for (int i = 1; i <= count; ++i) {
		std::shared_ptr<magicbox> it = CREATE_SHARED(magicbox);
		f.read((char*)&it->id, sizeof(magicbox));
		magicbox_data[it->id] = it;
	}
	f.close();
}

bool ItemDB::exists(uint32 id) {
	uint8 item_type = itemdb_type(id);

	if (item_type == ITEMDB_USE) {
		if (item_data.find(id) != item_data.end()) {
			return true;
		}
	}
	else if (item_type == ITEMDB_PART) {
		if (part_data.find(id) != part_data.end()) {
			return true;
		}
	}
	else if (item_type == ITEMDB_CLUB) {
		if (club_data.find(id) != club_data.end()) {
			return true;
		}
	}
	else if (item_type == ITEMDB_BALL) {
		if (ball_data.find(id) != ball_data.end()) {
			return true;
		}
	}
	else if (item_type == ITEMDB_CARD) {
		if (card_data.find(id) != card_data.end()) {
			return true;
		}
	}
	return false;
}

uint32 ItemDB::get_amount(uint32 id) {
	uint8 item_type = itemdb_type(id);

	if (item_type == ITEMDB_USE) {
		if (item_data.find(id) != item_data.end()) {
			return item_data[id]->status[0] == 0 ? 1 : item_data[id]->status[0];
		}
	}

	return 1;
}

std::pair<char, uint32> ItemDB::get_price(uint32 id) {
	uint8 item_type = itemdb_type(id);

	if (item_type == ITEMDB_USE) {
		if (item_data.find(id) != item_data.end()) {
			return std::make_pair(item_data[id]->base.price_type, item_data[id]->base.true_price > 0 ? item_data[id]->base.true_price : item_data[id]->base.price);
		}
	}
	else if (item_type == ITEMDB_CARD) {
		if (card_data.find(id) != card_data.end()) {
			return std::make_pair(card_data[id]->base.price_type, card_data[id]->base.true_price > 0 ? card_data[id]->base.true_price : card_data[id]->base.price);
		}
	}

	return std::make_pair(TYPE_INVALID, 0);
}

bool ItemDB::buyable(uint32 id) {
	uint8 item_type = itemdb_type(id);

	if (item_type == ITEMDB_USE) {
		if (item_data.find(id) != item_data.end()) {
			return item_data[id]->base.flag & 1;
		}
	}
	else if (item_type == ITEMDB_CARD) {
		if (card_data.find(id) != card_data.end()) {
			return card_data[id]->base.flag & 1;
		}
	}

	return false;
}

std::pair<bool, uint32> ItemDB::cardpack_data(uint32 id) {
	switch (id) {
	case 2092957696:
		return std::make_pair(true, 3);
		break;
	case 2092957700:
		return std::make_pair(true, 3);
		break;
	case 2092957701:
		return std::make_pair(true, 3);
		break;
	case 2092957703:
		return std::make_pair(true, 3);
		break;
	case 2092957706:
		return std::make_pair(true, 3);
	}

	return std::make_pair(false, 0);
}

std::vector<uint32> ItemDB::get_cardpack(uint32 id) {
	std::vector<uint32> list;

	// get group id
	auto group = group_db[id];

	if (!group) {
		return list;
	}

	std::pair<bool, uint32> data = cardpack_data(id);

	do {
		for (int i = 0; i < MAX_RAND_GROUP; ++i) {
			uint16 rand;
			if (!(int)group->random[i]->data.size())
				continue;
			rand = (rnd() % (int)group->random[i]->data.size()) - 1;

			uint32 card_typeid = group->random[i]->data[rand]->id;
			// find duplicate card
			if ( !VECTOR_FIND(list, card_typeid) ) {
				list.push_back(card_typeid);
			}
		}
	} while ( (int)list.size() < data.second);

	return list;
}

void ItemDB::pc_use_cardpack(pc* pc) {
	assert(pc);

	uint32 card_typeid = pc->read<uint32>();

	std::vector<uint32> pack_data = get_cardpack(card_typeid);

	if (!(uint8)pack_data.size()) {
		clif_pc_opencard_failed(pc);
		return;
	}

	ITEM_TRANSACTION card_ret = CREATE_SHARED(INV_TRANSACTION);
	char ret = pc->warehouse->delitem(pc, card_typeid, 1, &card_ret);

	if (ret != DELITEM_SUCCESS) {
		clif_pc_opencard_failed(pc);
		return;
	}

	Packet p;
	WTHEAD(&p, 0x154);
	WTIU32(&p, 0); // SUCCESS ??
	WTIU32(&p, card_ret->item_id);
	WTIU32(&p, card_ret->item_typeid);
	WTZERO(&p, 0x0C);
	WTIU32(&p, 1);
	WTZERO(&p, 0x20);
	WTIU16(&p, 1);
	WTIU08(&p, (uint8)pack_data.size());

	for (int i = 0; i < (uint8)pack_data.size(); ++i) {
		item item;
		item.type_id = pack_data[i];
		item.amount = 1;

		ITEM_TRANSACTION add_ret = CREATE_SHARED(INV_TRANSACTION);

		pc->warehouse->additem(pc, &item, false, &add_ret);

		WTIU32(&p, add_ret->item_id);
		WTIU32(&p, add_ret->item_typeid);
		WTZERO(&p, 0x0C);
		WTIU32(&p, add_ret->c0);
		WTZERO(&p, 0x20);
		WTIU16(&p, 1);
		WTIU32(&p, 1);
	}
	pc->send_packet(&p);
}

void ItemDB::read_random_group(const char* path_to_file){
	std::ifstream file(path_to_file);

	if (!file) {
		spdlog::get("console")->error("read_card_random(): cannot read file {}", path_to_file);
		return;
	}

	std::string line;

	while (std::getline(file, line)) {
		// find comment then replace with null
		std::size_t spos = line.find("//");
		if (spos != std::string::npos) {
			line.replace(spos, line.length() - spos, "");
		}
		// skip for empty line
		if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		std::string field;
		std::vector<std::string> seperated_fields;
		std::istringstream sfile(line);
		while (std::getline(sfile, field, ',')) {
			seperated_fields.push_back(field);
		}

		{
			int group_id = -1;
			unsigned int j, prob = 1;
			uint8 rand_group = 1;

			std::shared_ptr<item_group_random> random = nullptr;
			std::shared_ptr<item_group_entry> entry = std::make_shared<item_group_entry>();

			group_id = std::stoi(seperated_fields[0]);
			prob = std::stoi(seperated_fields[2]);
			rand_group = std::stoi(seperated_fields[4]);

			entry->id = std::stoi(seperated_fields[1]);
			entry->amount = std::stoi(seperated_fields[3]);
			entry->announce = 0;
			
			// checking group
			{
				// find group id
				auto it = group_db.find(group_id);

				// if there's no group id in container then create new one
				if (it == group_db.end()) {
					struct item_group_db* group = new item_group_db();
					group_db[group_id] = group;
					
					// generate new random group
					for (int k = 0; k < MAX_RAND_GROUP; ++k) {
						group->random.push_back(std::make_shared<item_group_random>());
					}
				}
			}

			// get group
			auto it = group_db[group_id];
			if (!it) throw "Error, unexpected to happen";
			
			// if it's a must item
			if (rand_group == 0) {
				it->must.push_back(entry);

				// skip to next process
				if (prob == 0) continue;
				rand_group = 0;
			}
			else {
				rand_group -= 1;
			}
			 
			random = it->random[rand_group];

			if (!random) throw "Error, Random group is nullptr!";
			
			for (j = 0; j < prob; ++j) {
				random->data.push_back(entry);
			}
		}
	}
}

void ItemDB::_GroupDB_destroy() { 

	for (auto &it : group_db) {
		// clear must entry
		it.second->must.clear();

		// clear random entry
		for (auto &rand : it.second->random) {
			rand->data.clear();
		}

		// clear random data
		it.second->random.clear();
	}

	// clear group data
	group_db.clear();
}