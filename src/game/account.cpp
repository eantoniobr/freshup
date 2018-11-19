#include "account.h"
#include "pc.h"
#include "clif.h"
#include "pc_manager.h"
#include "reader.h"
#include "channel.h"
#include "struct.h"
#include "static.h"
#include "inventory.h"

#include "../common/db.h"
#include "../common/packet.h"
#include "../common/utils.h"
#include "../common/typedef.h"


void pc_req_login(pc* pc) {
	std::string username = RTSTR(pc);
	int account_id = RTIU32(pc);
	RSKIP(pc, 6);
	std::string login_key = RTSTR(pc);
	std::string game_ver = RTSTR(pc);
	RSKIP(pc, 8);
	std::string game_key = RTSTR(pc);

	Statement stm(*get_session());
	stm << "SELECT account_id, userid, name, capability, sex FROM account WHERE account_id=? AND userid=? AND login_key=? AND game_key=?", 
		use(account_id), use(username), use(login_key), use(game_key), now;
	RecordSet rs(stm);
	
	if (rs.rowCount() <= 0) {
		Packet p;
		WTHEAD(&p, 630);
		WTIU32(&p, 300);
		pc->send_packet(&p);
		pc->disconnect();
		return;
	}

	pc->login_key = login_key;
	pc->game_key = game_key;
	pc->account_id_ = rs["account_id"];
	pc->username_ = rs["userid"].toString();
	pc->name_ = rs["name"].toString();
	pc->capability_ = rs["capability"];
	pc->sex_ = rs["sex"];
	
	sys_send_jerk(pc);
	sys_send_pc_data(pc);
	channel_manager->send_channel(pc);


	pc->warehouse->load_data(pc);
	pc->warehouse->send_data(pc, IV_CHAR);
	pc->warehouse->send_data(pc, IV_ALLITEM);
	pc->warehouse->send_data(pc, IV_CARD);
	pc->warehouse->send_data(pc, IV_EQUIPMENT);

	// temporarily sync money
	pc->sync_money();
}

void sys_send_pc_data(pc* pc) {
	Packet packet;
	packet.write<uint16>(0x44);
	packet.write<uint8>(0);
	packet.write<std::string>(config->read->Get("server", "client_Ver", "829.01"));
	packet.write<uint16>(65535);
	packet.write_string(pc->username_, 15);
	packet.write_null(7);
	packet.write_string(pc->name_, 16);
	packet.write_null(6);
	packet.write_null(21); // guild name
	packet.write_null(9); // guild image
	packet.write_null(7);
	packet.write<uint32>(pc->capability_);
	packet.write<uint32>(0);
	packet.write<uint32>(pc->connection_id_);
	packet.write_null(12);
	packet.write<uint32>(0); // guild id
	packet.write_hex(&pc_data1[0], sizeof(pc_data1));
	packet.write_string(pc->username_ + "@NT", 18);
	packet.write_null(0x6e);
	packet.write<uint32>(pc->account_id_);
	packet.write_null(239); // TODO: statistic
	packet.write_hex(&pc_data2[0], sizeof(pc_data2));
	packet.write_datetime();
	packet.write_hex(&pc_data3[0], sizeof(pc_data3));
	packet.write_null(8);
	packet.write<uint8>(1);
	packet.write<uint32>(0);
	packet.write<uint8>(8); // grandprix? 0 = no grandprix
	packet.write<uint16>(0);
	
	packet.write<uint32>(0); // guild id
	packet.write_string("", 20); // guild name
	packet.write_null(9);
	packet.write<uint32>(0); // guild total members
	packet.write_string("", 9); // guild image to file
	packet.write_null(3);
	packet.write_string("", 0x65); // guild notice
	packet.write_string("", 101); // guild introduce
	packet.write<uint32>(0); // my current position
	packet.write<uint32>(0); // guild leader account_id
	packet.write_string("", 22); // guild leader nick

	pc->send_packet(&packet);
}

void sys_send_jerk(pc* pc) {
	Packet packet;
	packet.write_hex(&game_login1[0], sizeof(game_login1));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login2[0], sizeof(game_login2));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login3[0], sizeof(game_login3));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login4[0], sizeof(game_login4));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login5[0], sizeof(game_login5));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login6[0], sizeof(game_login6));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login7[0], sizeof(game_login7));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login8[0], sizeof(game_login8));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login9[0], sizeof(game_login9));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login10[0], sizeof(game_login10));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login11[0], sizeof(game_login11));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login12[0], sizeof(game_login12));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login13[0], sizeof(game_login13));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login14[0], sizeof(game_login14));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login15[0], sizeof(game_login15));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login16[0], sizeof(game_login16));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login17[0], sizeof(game_login17));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login18[0], sizeof(game_login18));
	pc->send_packet(&packet);

	packet.reset();
	packet.write_hex(&game_login19[0], sizeof(game_login19));
	pc->send_packet(&packet);
}