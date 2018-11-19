#include <iostream>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "clif.h"
#include "pcs.h"
#include "reader.h"
#include "account.h"
#include "socket.h"
#include "../common/db.h"
#include "../common/crypto.h"

void signal_handler(int sig) {
	delete crypt;
	delete config;
	delete sclif;
	delete pc_manager;
	db_final();
}

int main(int argc, char *argv[]) {

	try {
#ifdef SIGBREAK
		signal(SIGBREAK, signal_handler);
#endif
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);

		// Initializing
		db_init();
		crypt = new Crypto();
		config = new Config();
		sclif = new clif();
		pc_manager = new PC_Manager();

		auto console = spdlog::stdout_color_mt("console");

		boost::asio::io_context io_context;
		Socket server(io_context, config->read->GetInteger("login", "port", 10201));
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "[ERROR]" << e.what() << std::endl;
		system("pause");
	}
}