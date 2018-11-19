#pragma once
#include <string>

class pc;

bool sys_name_validation(std::string& name);
void sys_send_data(pc* pc);

void pc_req_login(pc* pc);
void pc_checkup_name(pc* pc);
void pc_check_available(pc* pc);
void pc_req_create_char(pc* pc);
void pc_req_gamekey(pc* pc);