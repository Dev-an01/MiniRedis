CXXFLAGS = -std=gnu++17 -Wall -Wextra -O2   # gnu++17: container_of() uses typeof + statement exprs

all: server client

server: server.cpp hashtable.cpp zset.cpp avl.cpp heap.cpp threadpool.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

client: client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test: all
	$(CXX) $(CXXFLAGS) -o /tmp/test_avl    test_avl.cpp avl.cpp    && /tmp/test_avl
	$(CXX) $(CXXFLAGS) -o /tmp/test_offset test_offset.cpp avl.cpp && /tmp/test_offset
	$(CXX) $(CXXFLAGS) -o /tmp/test_heap   test_heap.cpp           && /tmp/test_heap
	./server & sleep 0.5; python3 test_cmds.py; rv=$$?; pkill -f '^./server'; exit $$rv

clean:
	rm -rf server client server.dSYM client.dSYM

.PHONY: all test clean
