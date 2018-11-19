#pragma once

#include <string>
#include <vector>

#include "../common/typedef.h"

class pc;
class Channel;
struct gamedata;
class Packet;

class game {
public:
	std::vector<pc*> pc_list;

	bool valid = true;

	uint32 owner_uid;
	uint16 roomId;
	uint32 vs_time;
	uint32 match_time;
	uint8 maxplayer;
	uint8 game_type;
	uint8 hole_total;
	uint8 map;
	uint8 mode;
	uint32 natural = 0;
	bool started = false;

	std::string name;
	std::string password;

	char gameKey[16];

	void sys_verify_pc(pc *pc);

	void genkey();
	void send(Packet *p);
	void gameupdate();
	void roomdata(pc *pc);
	void roomdata(Packet *p);

	virtual void send_pc_create(pc *pc) = 0;
	virtual void send_pc_join(pc *pc) = 0;
	virtual void sys_calc_pcslot() {};

	bool pc_remove(pc *pc);

	void sys_send_pcleave(pc *pc);
	void sys_weather(uint8 weather);
	void sys_inspec();

	Channel *channel = nullptr;
	game(std::shared_ptr<gamedata> const& data, uint16 room_id, std::string const& room_name, std::string const& room_pwd);

	void addmaster(pc *pc);
	void addpc(pc *pc);

	void pc_action(pc *pc);
	void pc_change_game_config(pc *pc);
	void pc_chat(pc *pc, Packet *p);
};

class game_chatroom : public game {
public:
	void send_pc_create(pc *pc);
	void send_pc_join(pc *pc);
	//void sys_calc_pcslot() = 0;

	game_chatroom(std::shared_ptr<gamedata> const& data, uint16 room_id, std::string const& room_name, std::string const& room_pwd);
};

class game_stroke : public game {
public:
	void send_pc_create(pc *pc);
	void send_pc_join(pc *pc);
	void sys_calc_pcslot();

	game_stroke(std::shared_ptr<gamedata> const& data, uint16 room_id, std::string const& room_name, std::string const& room_pwd);
};

enum rSetting {
	rsName = 0,
	rsPwd = 1,
	rsMap = 3,
	rsMode = 5,
	rsNatural = 14
};

enum roomErr {
	rFull = 2,
	rNotExist = 3,
	rPwdErr = 4,
	rFail = 7
};

enum rAction {
	rPCMoveAround = 0,
	rPCVSAnimate = 1,
	rPCAppear = 4,
	rPCPosture = 5,
	rPCMove = 6,
	rPCAction = 7,
	rPCAnimation = 8
};