#include "pc.h"
#include "gameplay.h"
#include "channel.h"
#include "../common/packet.h"

void pk_leave_game(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x4C);
	WTI16(&p, -1);
	pc->send_packet(&p);
}

void room_error(pc* pc, roomErr err) {
	Packet p;
	WTHEAD(&p, 0x49);
	WTIU08(&p, (uint8)err);
	pc->send_packet(&p);
}

void Channel::pc_create_game(pc* pc)
{
	std::shared_ptr<gamedata> data = CREATE_SHARED(gamedata);
	RTPOINTER(pc, &data->un1, sizeof gamedata);

	printf("vs = %d | match = %d | hole total = %d | max player %d | type = %d \n", data->vs_time, data->match_time, data->hole_total, data->max_player, data->game_type);

	int game_id = sys_get_game_id();

	if (game_id == -1)
		return; // Not enough game slot

	switch (data->game_type) {
	case gtStroke:
	{
		uint32 natural = RTIU32(pc);
		std::string name = RTSTR(pc);
		std::string pwd = RTSTR(pc);
		uint32 artifact = RTIU32(pc);
		game_list[game_id] = new game_stroke(data, game_id, name, pwd);
		game_list[game_id]->natural = natural;
	}
	break;
	case gtChatroom:
	{
		uint32 natural = RTIU32(pc);
		std::string name = RTSTR(pc);
		std::string pwd = RTSTR(pc);
		uint32 artifact = RTIU32(pc);
		game_list[game_id] = new game_chatroom(data, game_id, name, pwd);
		game_list[game_id]->natural = natural;
	}
	break;
	}

	if (!game_list[game_id]) {
		room_error(pc, rFail);
		return;
	}

	game_list[game_id]->channel = this;
	game_list[game_id]->addmaster(pc);
}

void Channel::pc_join_game(pc* pc) {
	uint16 req_id = RTIU16(pc);
	std::string req_pwd = RTSTR(pc);
	game* game = sys_getgame_byid(req_id);

	if ( game == nullptr || !game->valid) {
		room_error(pc, rNotExist);
		return;
	}

	if ((uint8)game->pc_list.size() >= game->maxplayer) {
		room_error(pc, rFull);
		return;
	}

	// if password not match or not gm
	if ( game->password.compare(req_pwd) != 0 && pc->capability_ != 4) {
		room_error(pc, rPwdErr);
		return;
	}

	game->addpc(pc);
}

void Channel::pc_leave_game(pc* pc) 
{
	game* game = pc->game;

	sys_veriy_game(game);
	
	if (game->pc_remove(pc)) {
		pc->game = nullptr;
		pc->game_id = -1;
		sys_pc_action(pc, lbUpdate);

		game->sys_send_pcleave(pc);

		if ((uint8)game->pc_list.size() > 0) 
			sys_game_action(game, gUpdate);

		game->sys_inspec();
		pk_leave_game(pc);
	}
}

void pc::change_equipment() 
{
	Packet p;
	uint8 action = RTIU08(this);
	uint32 value = RTIU32(this);

	WTHEAD(&p, 0x4B);
	WTIU32(&p, 0);
	WTIU08(&p, action);
	WTIU32(&p, connection_id_);

	switch (action) {
	case e_char:
		warehouse->write_current_char(&p);
		break;
	}

	if (action == e_char) {
		if (game == nullptr) return;
		game->send(&p);
	}
	else {
		send_packet(&p);
	}
}

void pc_roomaction(pc* pc) {
	if (pc->game == nullptr) 
		return;
	pc->game->pc_action(pc);
}

void game_setting(pc* pc) {
	if (pc->game == nullptr)
		return;

	pc->game->pc_change_game_config(pc);
}