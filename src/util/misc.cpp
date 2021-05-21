#include "util/misc.h"


template<typename LINE_FUNC>
static void HexDump_Internal(LINE_FUNC&& line_func, const void *ptr, size_t len, bool absolute)
{
	for (size_t i = 0; i < len; i += 0x10) {
		size_t n = (i + 0x10 < len ? 0x10 : len - i);
		assert(n > 0 && n <= 0x10);
		
		char buf[256];
		if (absolute) {
			V_sprintf_safe(buf, "%08x:  %*s  %*s  %*s  %*s  |%*s|\n", (uintptr_t)ptr + i, 11, "", 11, "", 11, "", 11, "", n, "");
		} else {
			V_sprintf_safe(buf, "%06x:  %*s  %*s  %*s  %*s  |%*s|\n", i, 11, "", 11, "", 11, "", 11, "", n, "");
		}
		
		for (size_t j = 0; j < n; ++j) {
			auto c = *(reinterpret_cast<const char *>(ptr) + i + j);
			
			auto l_nibble_to_hex_lower = [](char n){ return (n < 10 ? n + '0' : (n - 10) + 'a'); };
			auto l_nibble_to_hex_upper = [](char n){ return (n < 10 ? n + '0' : (n - 10) + 'A'); };
			
			int pos = (absolute ? 8 : 6) + (3 * j);
			     if (j <  4) pos += 2;
			else if (j <  8) pos += 3;
			else if (j < 12) pos += 4;
			else             pos += 5;
			
			buf[pos + 0] = l_nibble_to_hex_lower((c >> 4) & 0x0f);
			buf[pos + 1] = l_nibble_to_hex_lower((c >> 0) & 0x0f);
			
			if (!isprint(c)) c = '.';
			buf[(absolute ? 8 : 6) + 56 + j] = c;
		}
		
		line_func(buf);
	}
}

std::string HexDump(const void *ptr, size_t len, bool absolute)
{
	std::string str;
	HexDump_Internal([&str](const char *line){ str += line; }, ptr, len, absolute);
	return str;
}

void HexDumpToSpewFunc(void (*func)(const char *, ...), const void *ptr, size_t len, bool absolute)
{
	HexDump_Internal([=](const char *line){ (*func)("%s", line); }, ptr, len, absolute);
}
