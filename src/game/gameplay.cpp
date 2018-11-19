#include "gameplay.h"
#include "channel.h"
#include "pc.h"

#include "../common/utils.h"
#include "../common/packet.h"

game::game(std::shared_ptr<gamedata> const& data, uint16 room_id, std::string const& room_name, std::string const& room_pwd)
{
	roomId = room_id;
	vs_time = data->vs_time;
	match_time = data->match_time;
	maxplayer = data->max_player;
	game_type = data->game_type;
	hole_total = data->hole_total;
	map = data->map;
	mode = data->mode;

	name = room_name;
	password = room_pwd;

	genkey();
}

void game::addmaster(pc *pc)
{
	pc_list.push_back(pc);
	pc->game = this;
	pc->game_id = roomId;
	pc->game_role = 8;

	pc->game_position.init();
	pc->posture = 0;
	pc->animate = 0;

	sys_calc_pcslot();
	gameupdate();
	roomdata(pc);
	send_pc_create(pc);

	channel->sys_game_action(this, gCreate);
	channel->sys_pc_action(pc, lbUpdate);
}

void game::addpc(pc *pc)
{
	pc_list.push_back(pc);
	pc->game = this;
	pc->game_id = roomId;
	pc->game_role = 1;

	pc->game_position.init();
	pc->posture = 0;
	pc->animate = 0;

	sys_calc_pcslot();
	gameupdate();
	roomdata(pc);

	// send pc list
	channel->sys_game_action(this, gUpdate);
	channel->sys_pc_action(pc, lbUpdate);

	send_pc_join(pc);
}

void game::genkey() 
{
	for (int i = 0; i < sizeof gameKey; ++i) {
		gameKey[i] = rnd_value(1, 255);
	}
}

void game::send(Packet *p)
{
	std::for_each(pc_list.begin(), pc_list.end(), [p](pc* pc) {
		pc->send_packet(p);
	});
}

bool game::pc_remove(pc *pc)
{
	auto it = std::find(pc_list.begin(), pc_list.end(), pc);
	if (it == pc_list.end()) { return false; }

	pc_list.erase(it);

	return true;
}

void game::pc_action(pc *pc) {
	sys_verify_pc(pc);

	uint8 action = RTIU08(pc);

	Packet p;
	WTHEAD(&p, 0xC4);
	WTIU32(&p, pc->connection_id_);
	WTIU08(&p, action);

	switch (action) {
	case rPCMoveAround:
	{
		WTFLO(&p, RTFLO(pc));
	}
	break;
	case rPCVSAnimate:
	{
		WTCSTR(&p, RTSTR(pc));
	}
	break;
	case rPCAppear:
	{
		RTPOINTER(pc, &pc->game_position.x, sizeof Pos3D);
		WTPOINTER(&p, &pc->game_position.x, sizeof Pos3D);
	}
	break;
	case rPCMove:
	{
		Pos3D pos;
		RTPOINTER(pc, &pos.x, sizeof Pos3D);
		pc->game_position += pos;
		WTPOINTER(&p, &pos.x, sizeof Pos3D);
	}
	break;
	case rPCPosture:
	{
		pc->posture = RTIU32(pc);
		WTIU32(&p, pc->posture);
	}
	break;
	case rPCAction:
	{
		std::string action = RTSTR(pc);
		WTCSTR(&p, action);
	}
	break;
	case rPCAnimation:
	{
		pc->animate = RTIU32(pc);
		WTIU32(&p, pc->animate);
	}
	break;
	}

	send(&p);
}

void game::pc_change_game_config(pc *pc) {
	RSKIP(pc, 2);

	uint8 count = RTIU08(pc);

	for (int i = 1; i <= count; ++i) {
		uint8 action = RTIU08(pc);

		switch (action) {
		case rsName:
		{
			name = RTSTR(pc);
		}
		break;
		case rsPwd:
		{
			password = RTSTR(pc);
		}
		break;
		case rsMap:
		{
			map = RTIU08(pc);
		}
		break;
		case rsMode:
		{
			mode = RTIU08(pc);
		}
		break;
		case rsNatural:
		{
			natural = RTIU32(pc);
		}
		break;
		}
	}
	gameupdate();
	channel->sys_game_action(this, gUpdate);
}

void game::pc_chat(pc* pc, Packet *p) {
	sys_verify_pc(pc);
	send(p);
}

void game::sys_verify_pc(pc *pc)
{
	if (!VECTOR_FIND(pc_list, pc)) {
		throw "PC not found.";
	}
}

void game::sys_inspec()
{
	if ((uint8)pc_list.size() == 0) {
		valid = false;
		channel->sys_game_action(this, gDestroy);
	}
}

void game::sys_weather(uint8 weather)
{
	Packet p;
	WTHEAD(&p, 0x9E);
	WTIU08(&p, weather);
	WTIU16(&p, 0);
	send(&p);
}

void game::gameupdate() 
{
	Packet p;
	WTHEAD(&p, 0x4A);
	WTI16(&p, -1);
	WTIU08(&p, game_type);
	WTIU08(&p, map);
	WTIU08(&p, hole_total);
	WTIU08(&p, mode);
	WTIU32(&p, natural); // NATURAL MODE
	WTIU08(&p, maxplayer);
	WTIU08(&p, 30); // ??
	WTIU08(&p, 0); // ROOM IDLE?
	WTIU32(&p, vs_time);
	WTIU32(&p, match_time);
	WTIU32(&p, 0); // TROPHY TYPEID
	WTIU08(&p, (uint8)password.length() > 0 ? 0 : 1);
	WTCSTR(&p, name);
	send(&p);
}

void game::roomdata(Packet *p)
{
	WTFSTR(p, name, 40);
	WTZERO(p, 24);
	WTIU08(p, (uint8)password.length() > 0 ? 0 : 1);
	WTIU08(p, started ? 0 : 1);
	WTIU08(p, 0); // ORANGE ROOM ??
	WTIU08(p, maxplayer);
	WTIU08(p, (uint8)pc_list.size());
	WTPOINTER(p, &gameKey[0], sizeof gameKey);
	WTIU08(p, 0);
	WTIU08(p, 0x1E);
	WTIU08(p, hole_total);
	WTIU08(p, game_type);
	WTIU16(p, roomId);
	WTIU08(p, mode);
	WTIU08(p, map);
	WTIU32(p, vs_time);
	WTIU32(p, match_time);
	WTIU32(p, 0); // THROPHY
	WTIU08(p, 0); // IDILE?
	WTIU08(p, 0); // 1 = GM EVENT , 0 = NORMAL
	WTZERO(p, 0x4A);
	WTIU32(p, 100);
	WTIU32(p, 100);
	WTIU32(p, owner_uid);
	WTIU08(p, 0xFF); // PRACTICE?
	WTIU32(p, 0); // ARTIFACT
	WTIU32(p, natural);
	WTIU32(p, 0); // GRANDPRIX 1
	WTIU32(p, 0); // GRANDPRIX 2
	WTIU32(p, 0); // GRANDPRIX TIME
	WTIU32(p, 0); // IS GRANDPRIX
}

void game::sys_send_pcleave(pc *pc) {
	Packet p;
	WTHEAD(&p, 0x48);
	WTIU08(&p, 2);
	WTI16(&p, -1);
	WTIU32(&p, pc->connection_id_);
	send(&p);
}

void game::roomdata(pc *pc)
{
	Packet p;
	WTHEAD(&p, 0x49);
	WTIU16(&p, 0);
	roomdata(&p);
	pc->send_packet(&p);
}