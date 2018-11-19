#include "pc.h"
#include "account.h"
#include "../common/asio.hpp"

#include "Poco/Data/ODBC/ODBCException.h"
#include "spdlog/sinks/stdout_color_sinks.h"

pc::pc(int con_id, Session *session) : connection_id_(con_id), session_(session) {  }

pc::~pc() { spdlog::get("console")->warn("PC {} disconnected!", get_connection_id()); }

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

	try {
		switch (packet_id) {
		case req_login:
			pc_req_login(this);
			break;
		case req_gamekey:
			pc_req_gamekey(this);
			break;
		case req_name_available:
			pc_check_available(this);
			break;
		case req_checkup_name:
			pc_checkup_name(this);
			break;
		case req_char_creation:
			pc_req_create_char(this);
		default:
			for (int i = 0; i < bytes_recv - 5; ++i) {
				printf("%02x ", *(unsigned __int8*)(session_->get_receive_buffer() + i + 5));
			}
			break;
		}
	}
	catch (Poco::Data::ODBC::ODBCException& e) {
		spdlog::get("console")->critical(e.message());
		disconnect();
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
		disconnect();
	}
	catch (...) {
		spdlog::get("console")->critical("Unknown Exception Occured!");
		disconnect();
	}

}

void pc::set_accountid(int account_id) {
	account_id_ = account_id;
}

int pc::get_accountid() {
	return account_id_;
}

void pc::set_name(std::string name) {
	name_.assign(name);
}

std::string pc::get_name() {
	return name_;
}

void pc::set_username(std::string username) {
	username_ = username;
}

std::string pc::get_username() {
	return username_;
}