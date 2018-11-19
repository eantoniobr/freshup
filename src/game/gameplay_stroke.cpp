#include "gameplay.h"
#include "channel.h"
#include "pc.h"

#include "../common/utils.h"
#include "../common/packet.h"

game_stroke::game_stroke(std::shared_ptr<gamedata> const& data, uint16 room_id, std::string const& room_name, std::string const& room_pwd)
	: game(data, room_id, room_name, room_pwd){}

void game_stroke::send_pc_create(pc *pc) {
	Packet p;
	WTHEAD(&p, 0x48);
	WTIU08(&p, 0);
	WTI16(&p, -1);
	WTIU08(&p, (uint8)pc_list.size());
	pc->gamedata(&p, true);
	WTIU08(&p, 0);
	send(&p);
}

void game_stroke::send_pc_join(pc *pc) {
	// query all pc
	{
		Packet p;
		WTHEAD(&p, 0x48);
		WTIU08(&p, 0);
		WTI16(&p, -1);
		WTIU08(&p, (uint8)pc_list.size());
		for (auto &it : pc_list) {
			it->gamedata(&p, true);
		}
		WTIU08(&p, 0);
		pc->send_packet(&p);
	}

	// query this pc to all
	{
		Packet p;
		WTHEAD(&p, 0x48);
		WTIU08(&p, 1);
		WTI16(&p, -1);
		pc->gamedata(&p, true);
		send(&p);
	}
}

void game_stroke::sys_calc_pcslot()
{
	int slot = 1;

	for (auto &it : pc_list) {
		it->game_slot = slot;
		slot += 1;
	}
}