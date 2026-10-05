CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

all: version1/server version1/client version2_threadpool/server version2_threadpool/client

version1/server: version1/server.cpp common/protocol.h common/handler.h common/sha256.h
	$(CXX) $(CXXFLAGS) version1/server.cpp -o version1/server -pthread

version1/client: version1/client.cpp common/protocol.h common/sha256.h
	$(CXX) $(CXXFLAGS) version1/client.cpp -o version1/client

version2_threadpool/server: version2_threadpool/server.cpp version2_threadpool/threadpool.cpp version2_threadpool/threadpool.h common/protocol.h common/handler.h common/sha256.h
	$(CXX) $(CXXFLAGS) version2_threadpool/server.cpp version2_threadpool/threadpool.cpp -o version2_threadpool/server -pthread

version2_threadpool/client: version2_threadpool/client.cpp common/protocol.h common/sha256.h
	$(CXX) $(CXXFLAGS) version2_threadpool/client.cpp -o version2_threadpool/client

clean:
	rm -f version1/server version1/client version2_threadpool/server version2_threadpool/client

.PHONY: all clean
