#pragma once

#include "Poco/Data/RecordSet.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/ODBC/Connector.h"

using namespace Poco::Data::Keywords;
using Poco::Data::RecordSet;
using Poco::Data::Statement;

typedef std::unique_ptr<Poco:: Data::Session> session;

void db_init();
void db_final();
session get_session();

extern Poco::Data::SessionPool pooling;