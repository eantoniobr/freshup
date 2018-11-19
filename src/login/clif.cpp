#include "clif.h"
#include "pc.h"
#include "static.h"

#include "../common/typedef.h"
#include "../common/db.h"
#include "../common/packet.h"
#include "../common/utils.h"



clif* sclif = nullptr;

clif::clif() {}
clif::~clif() {}

void clif::login_msg(pc* pc, loginmsg type) {
	Packet p;

	switch(type) {
	case INCORRECT_PASSWORD:
		WTPHEX(&p, &incorrect_pwd[0], sizeof incorrect_pwd);
		break;
	case USER_NOT_FOUND:
		WTPHEX(&p, &user_notfound[0], sizeof user_notfound);
		break;
	case USER_STATE_BAN:
		WTPHEX(&p, &user_banned[0], sizeof user_banned);
		break;
	case MAINTENANCE_MODE:
		WTPHEX(&p, &sv_maintenance[0], sizeof sv_maintenance);
		break;
	}

	pc->send_packet(&p);
}

void clif::send_accountid(pc* pc) {
	Packet p;
	WTHEAD(&p, 3);
	WTIU32(&p, pc->get_accountid());
	pc->send_packet(&p);
}

void clif::send_firstset(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x0F);
	WTIU08(&p, 0);
	WTCSTR(&p, pc->get_username());
	pc->send_packet(&p);

	WRESET(&p);

	WTPHEX(&p, &FIRSTSET[0], sizeof FIRSTSET);
	pc->send_packet(&p);
}

void clif::send_name_taken(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x0E);
	WTIU32(&p, 0x0C);
	WTIU32(&p, 5100065); // code that show the name is taken
	pc->send_packet(&p);
}

void clif::send_name_available(pc* pc, std::string& name) {
	Packet p;
	WTHEAD(&p, 0x0E);
	WTIU32(&p, 0);
	WTCSTR(&p, name);
	pc->send_packet(&p);
}

void clif::send_name_validate_true(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x01);
	WTIU08(&p, 0xDA);
	pc->send_packet(&p);
}

void clif::send_name_validate_failed(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x0D);
	WTIU32(&p, 0x0C);
	WTIU32(&p, 5100049);
	pc->send_packet(&p);
}

void clif::send_auth_login(pc* pc) {
	Packet p;
	WTHEAD(&p, 0x10);
	WTCSTR(&p, pc->login_key);
	pc->send_packet(&p);
}

void clif::send_pc_data(pc* pc) {
	Packet p;
	WTHEAD(&p, 1);
	WTIU08(&p, 0);
	WTCSTR(&p, pc->get_username());
	WTIU32(&p, pc->get_accountid());
	WTIU32(&p, 0);
	WTIU32(&p, 0); // LEVEL
	WTZERO(&p, 6);
	WTCSTR(&p, pc->get_name());
	pc->send_packet(&p);
}

void clif::send_game_server(pc* pc) {
	Statement stm(*get_session());
	stm << "SELECT * FROM server WHERE server_type = 0", Poco::Data::Keywords::now;
	RecordSet rs(stm);
	bool done = rs.moveFirst();

	Packet p;
	WTHEAD(&p, 0x02);
	WTIU08(&p, (uint8)rs.rowCount());

	while (done) {
		WTFSTR(&p, rs["name"], 10);
		WTZERO(&p, 18);
		WTIU32(&p, 321005928); // MAYBE GAME VERSION
		WTZERO(&p, 8);
		WTIU32(&p, rs["server_id"]);
		WTIU32(&p, 10000); // MAX PLAYER
		WTIU32(&p, 0); // ONLINE PLAYER
		WTFSTR(&p, rs["ip"], 16);
		WTIU16(&p, 10592);
		WTIU16(&p, rs["port"]);
		WTZERO(&p, 3);
		WTIU08(&p, 8);
		WTZERO(&p, 2);
		WTIU32(&p, 1); // ANGELIC NUMBER
		WTIU16(&p, rs["img_event"]);
		WTZERO(&p, 6);
		WTIU16(&p, rs["img_no"]);
		done = rs.moveNext();
	}

	pc->send_packet(&p);
}

void clif::send_hotkey(pc* pc) {
	Statement stm(*get_session());
	stm << "SELECT * FROM hotkey WHERE account_id = ?", Poco::Data::Keywords::bind(pc->get_accountid()), Poco::Data::Keywords::now;
	RecordSet rs(stm);
	bool done = rs.moveFirst();

	Packet p;
	WTHEAD(&p, 6);
	while (done) {
		WTFSTR(&p, rs["hotkey1"], 64);
		WTFSTR(&p, rs["hotkey2"], 64);
		WTFSTR(&p, rs["hotkey3"], 64);
		WTFSTR(&p, rs["hotkey4"], 64);
		WTFSTR(&p, rs["hotkey5"], 64);
		WTFSTR(&p, rs["hotkey6"], 64);
		WTFSTR(&p, rs["hotkey7"], 64);
		WTFSTR(&p, rs["hotkey8"], 64);
		WTFSTR(&p, rs["hotkey9"], 64);
		done = rs.moveNext();
	}

	pc->send_packet(&p);
}