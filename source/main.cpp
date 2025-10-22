#include <libassert/assert.hpp>

#include <map>

void zoog(const std::map<std::string, int>& map) {
    DEBUG_ASSERT(map.contains("foo"), "expected key not found", map);
}


int main(int argc, const char** argv)
{
	zoog({ { "Foo", 43224 } });
	zoog({ { "Hello World", 434 } });
	return 0;
}