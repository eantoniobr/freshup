#include <iostream>
#include "../common/db.h"
#include "../common/crypto.h"
#include "../common/timer.h"
#include "../common/utils.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "socket.h"
#include "pc_manager.h"
#include "reader.h"
#include "account.h"
#include "channel.h"
#include "itemdb.h"

#include <boost/format.hpp>

void signal_handler(int sig) {
	delete crypt;
	delete config;
	delete pc_manager;
	delete channel_manager;
	delete itemdb;
	delete timer;
	db_final();
}


int main(int argc, char *argv[]) {

	auto console = spdlog::stdout_color_mt("console");
	try {
#ifdef SIGBREAK
		signal(SIGBREAK, signal_handler);
#endif
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);

		// Initializing
		db_init();
		timer = new TimerQueue();
		crypt = new Crypto();
		config = new Config();
		pc_manager = new PC_Manager();
		channel_manager = new ChannelManager();
		itemdb = new ItemDB();

		boost::asio::io_context io_context;
		Socket server(io_context, config->read->GetInteger("game", "port", 20201));
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "[ERROR]" << e.what() << std::endl;
		system("pause");
	}
}