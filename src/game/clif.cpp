#include "clif.h"
#include "pc.h" // pc class
#include "../common/utils.h"
#include "../common/packet.h" // Packet class

void clif_pc_opencard_failed(pc *pc) {
	Packet p;
	WTHEAD(&p, 0x154);
	WTIU32(&p, 1);
	pc->send_packet(&p);
}