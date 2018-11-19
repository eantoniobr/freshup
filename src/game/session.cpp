#include "session.h"

#include "../common/packet.h"
#include "../common/crypto.h"
#include "../common/utils.h"

#include "spdlog/spdlog.h" 
#include "pc.h"
#include "pc_manager.h"
#include "static.h"
Session::Session(boost::asio::io_context& io_context) : socket_(io_context), key_(0), pc_(pc_manager->pc_new(this)) {}

Session::pointer Session::create(boost::asio::io_context& io_context) {
	return pointer(new Session(io_context));
}

Session::~Session() {
	pc_manager->pc_remove(pc_);
}

boost::asio::ip::tcp::socket& Session::get_socket() {
	return socket_;
}

void Session::initialise() {
	// set no delay // to-do learn about it
	boost::asio::ip::tcp::no_delay option(true);
	socket_.set_option(option);

	// random key , uses for decrypt & encrypt packets
	key_ = rnd_value(1, 15);

	pc_->ip_ = socket_.remote_endpoint().address().to_string();

	spdlog::get("console")->info("Connection {}:{} connected with index {}", pc_->ip_, socket_.remote_endpoint().port(), pc_->connection_id_);

	// send key to client
	Packet p;
	WTIU08(&p, 0x00);
	WTIU16(&p, (uint16)pc_->ip_.length() + 8);
	WTPHEX(&p, &key_game[0], sizeof key_game);
	WTIU08(&p, key_);
	WTCSTR(&p, pc_->ip_);
	send_packet_undecrypt(&p);

	read_header();
}

void Session::disconnect() {
	try {
		socket_.close();
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
	}
}

void Session::handle_read_header(const boost::system::error_code &ec, std::size_t bytes_transferred) {
	if (!ec) {
		unsigned __int8 f_packet = *(unsigned __int8*)(receive_buffer_.get());
		unsigned __int16 packet_length = *(unsigned __int16*)(receive_buffer_.get() + 1);

		if (packet_length < 2) {
			disconnect();
			return;
		}

		receive_buffer_.reset(new unsigned char[packet_length + 4]);
		*(unsigned __int8*)(receive_buffer_.get()) = f_packet;
		*(unsigned __int16*)(receive_buffer_.get() + 1) = packet_length;
		*(unsigned __int8*)(receive_buffer_.get() + 3) = 0;

		auto  handler = boost::bind(&Session::handle_read_body, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
		boost::asio::async_read(socket_, boost::asio::buffer(receive_buffer_.get() + 4, packet_length), handler);
	}
	else {
		disconnect();
	}
}

void Session::handle_read_body(const boost::system::error_code &ec, std::size_t bytes_transferred) {
	if (!ec){
		// packet length
		unsigned __int16 bytes_recv = static_cast<unsigned __int16>(bytes_transferred);
		unsigned __int16 actual_bytes_recv = bytes_recv + 4; // we must add 4 bytes because we already read 4 bytes of header

		// decrypt packet note: we must add 4 to bytes_recv because we already read header
		crypt->Decrypt(receive_buffer_.get(), key_, actual_bytes_recv);

		pc_->handle_packet(actual_bytes_recv);

		read_header();
	}
	else {
		disconnect();
	}
}

void Session::read_header() {
	receive_buffer_.reset(new unsigned char[4]());

	auto handler = boost::bind(&Session::handle_read_header, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
	boost::asio::async_read(socket_, boost::asio::buffer(receive_buffer_.get(), 4), handler);
}

void Session::send_packet_undecrypt(Packet *packet) {
	try {
		if (!socket_.is_open()) {
			disconnect();
			return;
		}

		std::size_t packet_length = packet->get_length();
		unsigned char *packet_buffer = packet->get_buffer();

		auto handler = boost::bind(&Session::send_handler_undecrypt, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
		boost::asio::async_write(socket_, boost::asio::buffer(packet->get_buffer(), packet->get_length()), handler);
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
	}
}

void Session::send_packet(Packet *packet) {
	try {
		if (!socket_.is_open()) {
			disconnect();
			return;
		}

		std::size_t packet_length = packet->get_length();
		unsigned char *packet_buffer = packet->get_buffer();

		int newSize = 0;

		unsigned char *send_buffer;
		crypt->Encrypt(packet_buffer, key_, packet_length, &send_buffer, &newSize);

		auto handler = boost::bind(&Session::send_handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, send_buffer);
		boost::asio::async_write(socket_, boost::asio::buffer(send_buffer, newSize), handler);
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
	}
}

unsigned char* Session::get_receive_buffer() {
	return receive_buffer_.get();
}

void Session::send_handler(const boost::system::error_code &ec, std::size_t bytes_transferred, unsigned char *send_buffer) {
	try {
		crypt->_FreeMem(&send_buffer);
	}
	catch (std::exception& e) {
		spdlog::get("console")->critical(e.what());
	}
}