#pragma once
#include <string>
#include "session.h"

class Packet;

class pc {
	private:
		int connection_id_;
		int account_id_;
		std::string username_;
		std::string name_;

		/* this is for packets */
		int recv_length_;
		int recv_pos_;
		/* end of packets */

		Session *session_;

	public:
		pc(int con_id, Session *session);
		~pc(); 
		int get_connection_id();
		void disconnect();
		void skip(int amount);
		void send_packet(Packet *packet);
		void send_packet_undecrypt(Packet *packet);
		void handle_packet(unsigned short bytes_recv);

		void set_accountid(int account_id);
		int get_accountid();

		void set_name(std::string name);
		std::string get_name();

		void set_username(std::string username);
		std::string get_username();
		
		template<typename TYPE>
		TYPE read() {
			if (recv_length_ <= recv_pos_)
				return 0;

			if (sizeof(TYPE) > (recv_length_ - recv_pos_))
				return 0;

			TYPE val = *(TYPE *)(session_->get_receive_buffer() + recv_pos_);
			recv_pos_ += sizeof(TYPE);
			return val;
		}

		template<>
		bool read<bool>() {
			return read<signed char>() != 0;
		}

		template<>
		std::string read<std::string>() {
			int len = read<short>();

			if (recv_length_ <= recv_pos_)
				return "";

			if (len > (recv_length_ - recv_pos_)) { return ""; }
			std::string s((char*)session_->get_receive_buffer() + recv_pos_, len);
			recv_pos_ += len;
			return s;
		}

		std::string login_key;
		std::string game_key;
};

enum {
	req_login = 1,
	req_gamekey = 3,
	req_name_available = 6,
	req_checkup_name = 7,
	req_char_creation = 8
};