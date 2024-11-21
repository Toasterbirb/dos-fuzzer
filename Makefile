BIN=dos-fuzzer
PREFIX=/usr/local

CXX=g++
WARNINGS=-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Wundef -Werror -Wno-unused
CXXFLAGS=-O2 -std=c++20 -I./include -I./vendor/clipp/include $(WARNINGS)
LDFLAGS=

all: $(BIN)

$(BIN): $(patsubst %.cpp,%.o,$(wildcard ./src/*.cpp))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: ./src/%.cpp
	$(CXX) $(CXXFLAGS) -c $(LDFLAGS) $^

install:
	cp ./$(BIN) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	rm -f ./$(BIN) ./src/*.o

.PHONY: clean
