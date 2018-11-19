#include "mail.h"
#include "pc.h"
#include "itemdb.h"

#include "../common/utils.h"
#include "../common/packet.h"
#include "../common/db.h"

void pc_req_loadmail(pc* pc) {
	int page_select = RTIU32(pc);
	int page_total = 0;

	Poco::Data::Statement stm(*get_session());

	// select mail total from sql
	stm << "SELECT COUNT(mail_id) AS MAIL_TOTAL FROM mail WHERE account_id = ?", into(page_total), use(pc->account_id_), now;

	// mail page total
	page_total = (uint32)ceil(page_total * 1.0 / 20);

	// select mail data
	Poco::Data::Statement stm2(*get_session());
	stm2 << "SELECT A.mail_id, B.name, C.item_count,"
		<< "typeid = ISNULL(D.typeid, 0), set_typeid = ISNULL(D.set_typeid, 0), amount = ISNULL(D.amount, 0), day = ISNULL(D.day, 0), ucc, "
		<< "(CASE WHEN A.read_date IS NULL THEN 0 ELSE 1 END) AS mail_read "
		<< "FROM mail A "
		<< "LEFT JOIN account B ON A.sender = B.account_id "
		/* item mail total */
		<< "OUTER APPLY ( "
		<< "SELECT item_count = COUNT(mail_id) FROM mail_item WHERE mail_id = A.mail_id AND release_date IS NULL "
		<< ") C "
		/* first item */
		<< "OUTER APPLY ( "
		<< "SELECT TOP 1 typeid, set_typeid, amount, day, ucc FROM mail_item WHERE mail_id = A.mail_id AND release_date IS NULL "
		<< ") D "
		<< "WHERE A.account_id = ? AND delete_date IS NULL", use(pc->account_id_), now;

	Poco::Data::RecordSet rs(stm2);

	bool done = rs.moveFirst();

	Packet p;
	WTHEAD(&p, 0x211);
	WTIU32(&p, 0);
	WTIU32(&p, page_select);
	WTIU32(&p, page_total);
	WTIU32(&p, (uint32)rs.rowCount());

	while (done) {
		WTIU32(&p, rs["mail_id"]);
		WTFSTR(&p, rs["name"], 16);
		WTZERO(&p, 0x74);
		WTIU08(&p, rs["mail_read"]); // IS READ?
		WTIU32(&p, rs["item_count"]); // ITEM COUNT
		WTI32(&p, -1);
		WTIU32(&p, rs["set_typeid"] > 0 ? rs["set_typeid"] : rs["typeid"]); // ITEM TYPEID
		WTIU08(&p, rs["day"] > 0 ? 1 : 0); // TIMER?
		WTIU32(&p, rs["amount"]); // AMOUNT
		WTIU32(&p, rs["day"]);
		WTZERO(&p, 0x10);
		WTI32(&p, -1);
		WTIU32(&p, 0);
		WTFSTR(&p, rs["ucc"], 8); // UCC
		WTZERO(&p, 6);
		done = rs.moveNext();
	}

	pc->send_packet(&p);
}

void pc_req_readmail(pc* pc) {
	uint32 mail_id = RTIU32(pc);

	Packet p;

	{
		Poco::Data::Statement stm(*get_session());

		stm << "SELECT A.account_id, A.mail_id, A.message, CONVERT(VARCHAR, A.reg_date) AS date, B.name FROM mail A "
			<< "LEFT JOIN account B ON  B.account_id = A.sender "
			<< "WHERE A.account_id = ? AND A.mail_id = ?", use(pc->account_id_), use(mail_id), now;

		Poco::Data::RecordSet rs(stm);

		if (rs.rowCount() <= 0) return;

		WTHEAD(&p, 0x212);
		WTIU32(&p, 0);
		WTIU32(&p, rs["mail_id"]);
		WTCSTR(&p, rs["name"]);
		WTCSTR(&p, rs["date"]);
		WTCSTR(&p, rs["message"]);
		WTIU08(&p, 1);
	}

	{
		Poco::Data::Statement stm(*get_session());
		stm << "SELECT typeid, day, amount FROM mail_item WHERE mail_id = ?", use(mail_id), now;
		Poco::Data::RecordSet rs(stm);
		bool done = rs.moveFirst();

		WTIU32(&p, (uint32) rs.rowCount());

		while (done) {
			WTI32(&p, -1);
			WTIU32(&p, rs["typeid"]);
			WTIU08(&p, rs["day" > 0 ? 1 : 0]);
			WTIU32(&p, rs["amount"]);
			WTIU32(&p, rs["day"]);
			WTZERO(&p, 16);
			WTI32(&p, -1);
			WTIU32(&p, 0);
			WTIU32(&p, 0x30);
			WTZERO(&p, 10);
			done = rs.moveNext();
		}
	}

	pc->send_packet(&p);
}