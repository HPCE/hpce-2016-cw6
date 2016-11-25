CPPFLAGS += -std=c++11 -W -Wall -g -O3 -I include 
CPPFLAGS += -Wno-unused-parameter

LDLIBS += -ljpeg

bin/% : src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

reference_tools : bin/ref/simulator bin/tools/generate_heat_rect

user_simulator : bin/user/simulator
