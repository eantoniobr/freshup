#pragma once

#include <vector>
#include <memory>

#include "../common/typedef.h"
#include "../common/utils.h"

#define MAX_GAME_LIST 1000

class pc;
class Packet;
class game;

enum GAME_UPDATEACTION {
	gCreate = 1,
	gDestroy = 2,
	gUpdate = 3
};

enum PC_GAMEACTION {
	lbSend = 1,
	lbLeave = 2,
	lbUpdate = 3
};

enum {
	gtStroke = 0,
	gtChatroom = 2
};

enum {
	cmdWeather = 15
};

STRUCT_PACK(
struct gamedata {
	uint8 un1;
	uint32 vs_time;
	uint32 match_time;
	uint8 max_player;
	uint8 game_type;
	uint8 hole_total;
	uint8 map;
	uint8 mode;
});

class Channel {
public:
	uint8 id = 0;
	std::string name;
	uint16 maxplayer;
	std::vector<pc*> pc_list;

	game* game_list[MAX_GAME_LIST] = {0};

	uint16 pc_count() { return (uint16)pc_list.size(); }
	
	void sys_send_pc_list(pc* pc);
	void sys_send_game_list(pc* pc);
	void sys_get_pc_data(pc* pc, Packet* p);
	void sys_send_enter_lobby(pc* pc);
	void sys_send_leave_lobby(pc* pc);
	void sys_send(Packet* packet);
	void sys_verify_pc(pc* pc);
	void sys_veriy_game(game* game);
	game* sys_getgame_byid(uint32 room_id);

	void sys_game_action(game* game, GAME_UPDATEACTION const& action);
	void sys_pc_action(pc* pc, PC_GAMEACTION const& action);
	void sys_gm_command(pc* pc);
	int sys_get_game_id();

	void pc_enter_lobby(pc* pc);
	void pc_leave_lobby(pc* pc);
	void pc_quit_lobby(pc* pc);
	void pc_send_message(pc* pc);

	void pc_create_game(pc* pc);
	void pc_join_game(pc* pc);
	void pc_leave_game(pc* pc);

	void game_destroy();

	Channel();
};

class ChannelManager {
public:
	std::vector<Channel*> channel_list;
	ChannelManager();
	~ChannelManager();

	void pc_select_channel(pc* pc);

	Channel* get_channel_ById(uint8 id);

	void send_channel(pc* pc);
	void send_channel_full(pc* pc);
	void send_success(pc* pc);
};

extern ChannelManager* channel_manager;