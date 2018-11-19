#include "shop.h"
#include "pc.h"
#include "itemdb.h"
#include "../common/packet.h"

void pc_req_entershop(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x20E);
	WTZERO(&p, 8);
	pc->send_packet(&p);
}

void pc_req_buyitem(pc* pc) {
	uint8 buy_type = pc->read<uint8>();
	
	switch (buy_type) {
	case BUY_NORMAL:
		pc_buyitem_normal(pc);
		break;
	case BUY_RENT:
		pc_buyitem_rent(pc);
		break;
	}
}

void pc_buyitem_normal(pc* pc) {
	uint16 buy_total_item = RTIU16(pc);
	uint32 item_amount = 0, pang_total = 0, cookie_total = 0;

	buy_data item_buy[MAX_ITEM_BUY];

	for (int i = 0; i < buy_total_item; ++i) {
		pc->read((char*)&item_buy[i].un1, sizeof buy_data);
	}

	for (int i = 0; i < buy_total_item; ++i) {
		// item exist?
		if (!itemdb->exists(item_buy[i].item_typeid)) {
			send_msg(pc, BUY_FAIL);
			return;
		}
		// item buyable?
		else if (!itemdb->buyable(item_buy[i].item_typeid)) {
			send_msg(pc, CANNOT_BUY_ITEM);
			return;
		}

		// sum pang and cookie
		std::pair<char, uint32> price_data = itemdb->get_price(item_buy[i].item_typeid);

		if (price_data.first == TYPE_PANG || price_data.first == TYPE_PANG2) {
			item_buy[i].amount = itemdb->get_amount(item_buy[i].item_typeid) * item_buy[i].amount; // get real amount of the item : type pang
			pang_total += price_data.second * item_buy[i].amount;
		}
		else if (price_data.first == TYPE_COOKIE) {
			item_buy[i].amount = itemdb->get_amount(item_buy[i].item_typeid); // Item Cookie Amount might be static
			cookie_total += price_data.second;
		}
		else {
			throw "Price type is invalid.";
		}

		printf("pang total %d, cookie total %d , amount is =%d\n", pang_total, cookie_total, itemdb->get_amount(item_buy[i].item_typeid));

		item item;
		item.type_id = item_buy[i].item_typeid;
		item.amount = item_buy[i].amount;

		// validate that pc can have this item
		switch (pc->warehouse->additem(pc,  &item, true)) {
		case CHECKITEM_PASS:
			break;
		case CHECKITEM_FAIL:
			send_msg(pc, BUY_FAIL);
			return;
			break;
		case CHECKITEM_OVERLIMIT:
			send_msg(pc, TOO_MUCH_ITEM);
			return;
			break;
		case CHECKITEM_DUPLICATED:
			send_msg(pc, ALREADY_HAVEITEM);
			return;
			break;
		}
	}
	
	for (int i = 0; i < buy_total_item; ++i) {
		item item;
		item.type_id = item_buy[i].item_typeid;
		item.amount = item_buy[i].amount;

		ITEM_TRANSACTION tran = CREATE_SHARED(INV_TRANSACTION);
		pc->warehouse->additem(pc, &item, false, &tran);
		buyitem_result(pc, &tran, 0, 0);
	}

	send_msg(pc, BUY_SUCCESS, true);
}

void pc_buyitem_rent(pc* pc) {
	uint16 total = pc->read<uint16>();
}

void buyitem_result(pc* pc, ITEM_TRANSACTION* tran, uint16 day, uint8 flag) {
	Packet p;
	WTHEAD(&p, 0xAA);
	WTIU16(&p, 1);
	WTIU32(&p, (*tran)->item_typeid);
	WTIU32(&p, (*tran)->item_id);
	WTIU16(&p, day);
	WTIU08(&p, flag);
	WTIU16(&p, (*tran)->new_amount);
	p.write_datetime((*tran)->timestamp_end);
	WTFSTR(&p, (*tran)->ucc_unique, 9);
	WTIU64(&p, 1000000); // pc pang
	WTIU64(&p, 1000000); // pc cookie;
	pc->send_packet(&p);
}

void send_msg(pc* pc, uint32 code, bool success) {
	Packet p;
	p.write<uint16>(0x68);
	p.write<uint32>(code);

	if (success) {
		p.write<uint64>(100000000);
		p.write<uint64>(100000000);
	}

	pc->send_packet(&p);
}