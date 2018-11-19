#include "utils.h"
#include <random>

std::mt19937 generator;
std::uniform_int_distribution<int32> int31_distribution;
std::uniform_int_distribution<uint32> uint32_distribution;

/// Initializes the random number generator
void rnd_init(void) {
	std::random_device device;
	generator = std::mt19937(device());
	int31_distribution = std::uniform_int_distribution<int32>(0, 0x7FFFFFFF);
	uint32_distribution = std::uniform_int_distribution<uint32>(0, 0xFFFFFFFF);
}

int32 rnd(void) {
	return int31_distribution(generator);
}

uint32 rnd_uint32(void) {
	return uint32_distribution(generator);
}

double rnd_uniform(void) {
	return rnd_uint32() * (1.0 / 4294967296.0);
}

int32 rnd_value(int32 min, int32 max) {
	if (min >= max) {
		return min;
	}

	return min + (int32)(rnd_uniform() * (max - min + 1));
}

std::string rnd_str(int max_length, std::string possible_chars) {
	std::uniform_int_distribution<> dist(0, possible_chars.size() - 1);
	std::string ret = "";
	for (int i = 0; i < max_length; i++) {
		int random_index = dist(generator);
		ret += possible_chars[random_index];
	}
	return ret;
}

uint8 itemdb_type(uint32 id) {
	return static_cast<uint8>((id & 0xfc000000) / pow(2, 26));
}