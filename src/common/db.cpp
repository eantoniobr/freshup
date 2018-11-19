#include "db.h"

Poco::Data::SessionPool pooling("ODBC", "DSN=PANGYA_DSN;Uid=sa;Pwd=123456", 1, 32);

session get_session() {
	session sess = std::make_unique<Poco::Data::Session>(pooling.get());
	sess->setFeature("forceEmptyString", true);
	return sess;
}

void db_init() {
	Poco::Data::ODBC::Connector::registerConnector();
}

void db_final() {
	pooling.shutdown();
}