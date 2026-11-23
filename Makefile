CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -DNDEBUG
LDFLAGS  := -lSDL3 -lGL -lm -lpthread

SRCS := src/main.cpp src/gl_loader.cpp src/mesh.cpp src/world.cpp src/hud.cpp src/audio.cpp src/game.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := aetherwing

.PHONY: all clean run verify

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

verify: $(TARGET)
	SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ./$(TARGET) --benchmark 90

clean:
	rm -f $(OBJS) $(TARGET)
