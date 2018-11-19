#include "channel.h"
#include "gameplay.h"
#include "pc.h"

#include "../common/packet.h"
#include "../common/timer.h"

#include "spdlog/sinks/stdout_color_sinks.h"

ChannelManager* channel_manager = nullptr;

ChannelManager::ChannelManager() 
{
	for (uint8 i = 1; i <= 4; ++i) 
	{
		Channel* ch(new Channel());
		ch->id = i;
		ch->name = "Free#" + std::to_string(i);
		ch->maxplayer = 255;

		spdlog::get("console")->info("{}. {} MaxPlayer: {}", i, ch->name, ch->maxplayer);

		channel_list.push_back(ch);
	}
}

ChannelManager::~ChannelManager() 
{
	std::for_each(channel_list.begin(), channel_list.end(), [](Channel *ch) { delete ch; });
}

void ChannelManager::pc_select_channel(pc* pc) 
{
	uint8 channel_id = RTIU08(pc);
	Channel* ch = get_channel_ById(channel_id);

	if (ch == nullptr) {
		pc->disconnect();
		return;
	}

	if (ch->pc_count() >= ch->maxplayer) {
		send_channel_full(pc);
		return;
	}

	ch->pc_list.push_back(pc);
	pc->channel_ = ch;
	send_success(pc);
}

Channel* ChannelManager::get_channel_ById(uint8 id) 
{
	auto it = std::find_if(channel_list.begin(), channel_list.end(), [&id](Channel* m) {
		return m->id == id;
	});

	if (VECTOR_FINDIF(channel_list, it)) {
		return (*it);
	}

	return nullptr;
}

void ChannelManager::send_channel(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0x4D);
	WTIU08(&p, (uint8)channel_list.size());
	for (auto &it : channel_list) {
		WTFSTR(&p, it->name, 10);
		WTZERO(&p, 0x36);
		WTIU16(&p, it->maxplayer);
		WTIU16(&p, it->pc_count());
		WTIU08(&p, it->id);
		WTIU32(&p, 1);
		WTIU32(&p, 0);
	}
	pc->send_packet(&p);
}

void ChannelManager::send_channel_full(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0x4E);
	WTIU08(&p, 2);
	pc->send_packet(&p);
}

void ChannelManager::send_success(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0x4E);
	WTIU08(&p, 1);
	pc->send_packet(&p);
}

/* Channel */

Channel::Channel()
{
	timer->add(10000, true, [this](bool abort) {
		game_destroy();
	});
}

void Channel::game_destroy(void)
{
	for (int i = 0; i < MAX_GAME_LIST; i++) {
		if (game_list[i] && !game_list[i]->valid) {
			NULL_POINTER(game_list[i]);
		}
	}
}

void Channel::pc_enter_lobby(pc* pc) 
{
	sys_verify_pc(pc);

	if (pc->channel_in_) {
		return;
	}

	pc->channel_in_ = true;

	sys_send_pc_list(pc);
	sys_send_game_list(pc);
	sys_pc_action(pc, lbSend);
	sys_send_enter_lobby(pc);
}

void Channel::pc_leave_lobby(pc* pc) 
{
	sys_verify_pc(pc);

	if (!pc->channel_) {
		return;
	}

	pc->channel_in_ = false;
	sys_pc_action(pc, lbLeave);
	sys_send_leave_lobby(pc);
}

void Channel::pc_quit_lobby(pc* pc) 
{
	sys_verify_pc(pc);

	auto it = std::find( pc_list.begin(), pc_list.end(), pc);

	if (it == std::end(pc_list)) {
		return;
	}

	if (pc->channel_in_) {
		sys_pc_action(pc, lbLeave);
	}

	pc_list.erase(it);
}

void Channel::pc_send_message(pc* sd)
{
	struct item item;
	item.amount = 17;
	item.day_amount = 0;
	item.flag = 0;
	item.item_type = 2;
	item.type_id = 2092957703;


	sys_verify_pc(sd);

	sd->read<uint32>();
	std::string name = RTSTR(sd);
	std::string msg = RTSTR(sd);

	if ( !STRCMP(sd->name_, name) ) {
		return;
	}

	Packet p;
	WTHEAD(&p, 0x40);
	WTIU08(&p, sd->capability_ == 4 ? 0x80 : 0);
	WTCSTR(&p, sd->name_);
	WTCSTR(&p, msg);

	if (sd->game == nullptr) {
		std::for_each(pc_list.begin(), pc_list.end(), [&p](pc* pc) {
			if (pc->game == nullptr && pc->channel_in_) {
				pc->send_packet(&p);
			}
		});
	}
	else {
		sd->game->pc_chat(sd, &p);
	}
}

game* Channel::sys_getgame_byid(uint32 room_id)
{
	if (game_list[room_id]) {
		return game_list[room_id];
	}

	return nullptr;
}

void Channel::sys_gm_command(pc* pc) {

	if (pc->capability_ != 4) throw "You're not not GM!";

	uint16 command = RTIU16(pc);

	switch (command) {
	case cmdWeather:
	{
		uint8 weather = RTIU08(pc);
		if (pc->game) {
			pc->game->sys_weather(weather);
		}
	}
	break;
	}
}

void Channel::sys_game_action(game* game, GAME_UPDATEACTION const& action) 
{
	sys_veriy_game(game);
	Packet p;
	WTHEAD(&p, 0x47);
	WTIU08(&p, 1);
	WTIU08(&p, (uint8)action);
	WTI16(&p, -1);
	game->roomdata(&p); // get game data
	sys_send(&p);
}

void Channel::sys_send_pc_list(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0x46);
	WTIU08(&p, 4); // Show Player

	int i = 0;

	for (auto &pc : pc_list) {
		if (pc->channel_in_) {
			i += 1;
		}
	}

	WTIU08(&p, i);

	for (auto pc : pc_list) {
		if (pc->channel_in_) {
			sys_get_pc_data(pc, &p);
		}
	}

	pc->send_packet(&p);
}

void Channel::sys_send_game_list(pc* pc)
{
	int16 count = 0;

	for (int i = 0; i < MAX_GAME_LIST; ++i) {
		if (game_list[i] && game_list[i]->valid) {
			count += 1;
		}
	}

	Packet p;
	WTHEAD(&p, 0x47);
	WTIU16(&p, count);
	WTI16(&p, -1);

	for (int i = 0; i < MAX_GAME_LIST; ++i) {
		if (game_list[i] && game_list[i]->valid) {
			game_list[i]->roomdata(&p);
		}
	}

	pc->send_packet(&p);
}

void Channel::sys_pc_action(pc* pc, PC_GAMEACTION const& action) 
{
	Packet p;
	WTHEAD(&p, 0x46);
	WTIU08(&p, (uint8)action);
	WTIU08(&p, 1);
	sys_get_pc_data(pc, &p);
	sys_send(&p);
}

void Channel::sys_get_pc_data(pc* pc, Packet* p) 
{
	WTIU32(p, pc->account_id_);
	WTIU32(p, pc->connection_id_);
	WTI16(p, pc->game_id);
	WTFSTR(p, pc->name_, 16);
	WTZERO(p, 6);
	WTIU08(p, 35); // LEVEL
	WTIU32(p, 0); // GM VISIBLE
	WTIU32(p, 0); // TITLE TYPEID
	WTIU32(p, 1000); // WHAT??
	WTIU08(p, pc->sex_);
	WTIU32(p, 0); // GUILD ID
	WTFSTR(p, "", 9); // GUILD IMAGE
	WTZERO(p, 3);
	WTIU08(p, 1); // VIP
	WTZERO(p, 6);
	WTFSTR(p, pc->username_ + "@NT", 18);
	WTZERO(p, 0x6E);
}


void Channel::sys_send_enter_lobby(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0xF5);
	pc->send_packet(&p);
}

void Channel::sys_send_leave_lobby(pc* pc) 
{
	Packet p;
	WTHEAD(&p, 0xF6);
	pc->send_packet(&p);
}

void Channel::sys_send(Packet* packet) 
{
	std::for_each(pc_list.begin(), pc_list.end(), [&packet](pc* pc) {
		pc->send_packet(packet);
	});
}

void Channel::sys_verify_pc(pc* pc) 
{
	if (pc == nullptr)
		return throw ChannelNotFound();

	if (!VECTOR_FIND(pc_list, pc))
		throw ChannelNotFound();
}

void Channel::sys_veriy_game(game* game)
{
	if (game == nullptr)
		return throw GameNotFoundOnChannel();

	int game_id = game->roomId;

	if (game_list[game_id] != game) {
		throw GameNotFoundOnChannel();
	}
}

int Channel::sys_get_game_id() {
	for (int i = 0; i < MAX_GAME_LIST; ++i) {
		if (game_list[i] == nullptr) {
			return i;
		}
	}

	return -1;
}