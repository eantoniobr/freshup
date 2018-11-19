#pragma once
#include <string>
#include "session.h"
#include "inventory.h"
#include "exception.h"

#include "../common/utils.h"

class Packet;
class Channel;
class game;


#pragma pack(push, 1)
struct Pos3D {
	float x;
	float y;
	float z;
	void operator+=(const Pos3D& other) {
		x += other.x;
		y += other.y;
		z = other.z;
	}
	void init() {
		x = 0;
		y = 0;
		z = 0;
	}
};
#pragma pack(pop)

class pc {
	public:
		int recv_length_;
		int recv_pos_;
		Session *session_;
		std::string ip_;

		int connection_id_;
		int account_id_;

		std::shared_ptr<PC_Warehouse> warehouse;

		std::string login_key;
		std::string game_key;
		std::string username_;
		std::string name_;
		int sex_;
		int capability_ = 0;

		int test = 1;
		// temporarily sync money (pang, cookie)
		void sync_money();

		Channel* channel_ = nullptr;
		bool channel_in_ = false;

		game* game = nullptr;
		__int16 game_id = -1;
		uint8 game_role = 1;
		uint8 game_slot;
		bool game_ready = false;
		Pos3D game_position;
		uint32 posture = 0;
		uint32 animate = 0;

		pc(int con_id, Session *session);
		~pc(); 
		int get_connection_id();
		void disconnect();
		void skip(int amount);
		void send_packet(Packet *packet);
		void send_packet_undecrypt(Packet *packet);
		void handle_packet(unsigned short bytes_recv);
		void gamedata(Packet* p, bool with_equip = false);

		void change_equipment();
		 

		template<typename TYPE> TYPE read() {
			if ((recv_length_ - recv_pos_) < sizeof TYPE) throw ReadPacketError();
			TYPE val;
			memcpy(&val, (session_->get_receive_buffer() + recv_pos_), sizeof TYPE);
			recv_pos_ += sizeof TYPE;
			return val;
		}

		template<> bool read<bool>() {
			return read<signed char>() != 0;
		}

		void read(char* data, std::size_t size) {
			if ((recv_length_ - recv_pos_) < size) throw ReadPacketError();
			memcpy(data, (session_->get_receive_buffer() + recv_pos_), size);
			recv_pos_ += size;
		}

		template<> std::string read<std::string>() {
			int len = read<short>();

			if (recv_length_ <= recv_pos_)
				return "";

			if (len > (recv_length_ - recv_pos_)) { return ""; }
			std::string s((char*)session_->get_receive_buffer() + recv_pos_, len);
			recv_pos_ += len;
			return s;
		}
};

enum packet {
	pc_login = 2,
	pc_send_message = 3,
	pc_select_channel = 4,
	pc_create_game_ = 8,
	pc_join_game = 9,
	pc_game_config = 10,
	pc_change_equipment = 12,
	pc_leave_room = 15,
	pc_room_action = 99,
	pc_enter_lobby_ = 129,
	pc_leave_lobby_ = 130,
	pc_gm_command = 143,
	pc_open_cardpack = 202, 

	pc_buyitem = 29,
	pc_enter_shop = 320,

	/* mail system */
	pc_loadmail_ = 0x143,
	pc_readmail_ = 0x144
};

enum {
	e_caddie = 1,
	e_club = 3,
	e_char = 4,
	e_mascot = 5,
	e_start = 7
};

// pc game function
void pc_roomaction(pc* pc);
void game_setting(pc* pc);