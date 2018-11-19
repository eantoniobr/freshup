#include "pc.h"
#include "account.h"
#include "channel.h"
#include "shop.h"
#include "mail.h"

#include "Poco/Data/ODBC/ODBCException.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "../common/packet.h"
#include "../common/asio.hpp"

pc::pc(int con_id, Session *session) :
	connection_id_(con_id), 
	session_(session),
	account_id_(0),
	warehouse(CREATE_SHARED(PC_Warehouse))
{}

pc::~pc() { 

	// save inventory data
	warehouse->savedata(this);

	if (channel_) {
		if (game != nullptr) {
			channel_->pc_leave_game(this);
		}

		channel_->pc_quit_lobby(this);
		channel_ = nullptr;
	}

	spdlog::get("console")->warn("PC {} disconnected!", get_connection_id()); 
}

int pc::get_connection_id() {
	return connection_id_;
}

void pc::skip(int amount) {
	recv_pos_ += amount;
}

void pc::disconnect() {
	session_->disconnect();
}

void pc::send_packet(Packet *packet) {
	session_->send_packet(packet);
}

void pc::send_packet_undecrypt(Packet *packet) {
	session_->send_packet_undecrypt(packet);
}

void pc::handle_packet(unsigned short bytes_recv) {
	recv_pos_ = 0;
	recv_length_ = bytes_recv;

	// skip unuse packets
	skip(5);

	__int16 packet_id = read<__int16>();

	for (int i = 0; i < bytes_recv - 5; ++i) {
		printf("%02x ", *(unsigned __int8*)(session_->get_receive_buffer() + i + 5));
	}
	printf("\n");

	Poco::Timestamp now;

	try {
		switch (packet_id) {
		case packet::pc_send_message:
			channel_->pc_send_message(this);
			break;
		case packet::pc_login:
			pc_req_login(this);
			break;
		case packet::pc_select_channel:
			channel_manager->pc_select_channel(this);
			break;
		case packet::pc_create_game_:
			channel_->pc_create_game(this);
			break;
		case packet::pc_join_game:
			channel_->pc_join_game(this);
			break;
		case pc_game_config:
			game_setting(this);
			break;
		case packet::pc_leave_room:
			channel_->pc_leave_game(this);
			break;
		case packet::pc_room_action:
			pc_roomaction(this);
			break;
		case packet::pc_change_equipment:
			change_equipment();
			break;
		case packet::pc_enter_lobby_:
			channel_->pc_enter_lobby(this);
			break;
		case packet::pc_leave_lobby_:
			channel_->pc_leave_lobby(this);
			break;
		case packet::pc_gm_command:
			channel_->sys_gm_command(this);
			break;
		case packet::pc_enter_shop:
			pc_req_entershop(this);
			break;
		case packet::pc_buyitem:
			pc_req_buyitem(this);
			break;
		case packet::pc_open_cardpack:
			itemdb->pc_use_cardpack(this);
			break;
		case packet::pc_loadmail_:
			pc_req_loadmail(this);
			break;
		case packet::pc_readmail_:
			pc_req_readmail(this);
			break;
		default:
			break;
		}
	}
	catch (Poco::Data::ODBC::ODBCException& e) {
		spdlog::get("console")->critical(e.message());
		disconnect();
	}
	catch (ChannelNotFound& e) {
		spdlog::get("console")->critical(e.what());
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
		disconnect();
	}
	catch (...) {
		spdlog::get("console")->critical("Unknown Exception Occured!");
		disconnect();
	}

	Poco::Timestamp::TimeDiff diff = now.elapsed() / 1000;
	spdlog::get("console")->info("PC last processed took {} = {}ms", now.elapsed(), diff);

}

void pc::gamedata(Packet* p, bool with_equip) {
	WTIU32(p, connection_id_);
	WTFSTR(p, name_, 0x10);
	WTZERO(p, 6);
	WTFSTR(p, "", 0x15); // guild name
	WTIU08(p, game_slot);
	WTIU32(p, 0);
	WTIU32(p, 0); // title typeid
	WTIU32(p, warehouse->char_typeid_equiped());
	WTZERO(p, 0x14); // ?
	WTIU32(p, 0); // title typeid
	WTIU08(p, game_role);
	WTIU08(p, game_ready ? 2 : 0);
	WTIU08(p, 31); // level
	WTIU08(p, 0); // GM
	WTIU08(p, 10); // ??
	WTIU32(p, 0); // GUILD ID
	WTFSTR(p, "guildmark", 9);
	WTIU32(p, 0);
	WTIU32(p, account_id_);
	WTIU32(p, animate);
	WTIU16(p, 41485);
	WTIU32(p, posture);
	WTFLO(p, game_position.x);
	WTFLO(p, game_position.y);
	WTFLO(p, game_position.z);
	WTIU32(p, 0);
	WTFSTR(p, "STORE NAME", 0x1F);
	WTZERO(p, 0x21);
	WTIU32(p, 0); // MASCOT TYPEID
	WTIU08(p, 0); // PANG MAS ENABLE
	WTIU08(p, 0); // NITRO PANG MAS ENABLE
	WTIU32(p, 0); // ?
	WTFSTR(p, username_ + "@NT", 18);
	WTZERO(p, 0x6E);
	WTIU32(p, 0); // THOPHY ?
	WTIU32(p, 66); // ??

	if (with_equip) {
		warehouse->write_current_char(p);
	}
}

void pc::sync_money() {
	Packet packet;
	packet.write<uint16>(0xc8);
	packet.write<uint32>(100000000);
	packet.write_null(12);
	send_packet(&packet);

	packet.reset();
	packet.write<uint16>(0x96);
	packet.write<uint32>(100000000);
	packet.write<uint32>(0);
	send_packet(&packet);
}