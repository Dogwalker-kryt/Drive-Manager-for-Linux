CXX := g++
PKGCONFIG := pkg-config
CXXFLAGS := -std=c++17 -O2 `$(PKGCONFIG) --cflags gtk+-3.0`
LIBS := `$(PKGCONFIG) --libs gtk+-3.0`

SRCS := main.cpp drives.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := DriveMgr_GUI

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	mkdir -p $(HOME)/.local/bin
	cp $(TARGET) $(HOME)/.local/bin/$(TARGET)

.PHONY: all clean install
