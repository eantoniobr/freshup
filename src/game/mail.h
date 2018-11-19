#pragma once
#include <string>
#include "../common/typedef.h"

class pc;

struct mail_data {
	uint32 mail_id;
	std::string sender;
	std::string subject;
	std::string message;
};

void pc_req_loadmail(pc* pc);
void pc_req_readmail(pc* pc);