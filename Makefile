LD_FLAGS := -lboost_system -lboost_filesystem
CC_FLAGS := -std=c++0x

INCLUDE_DIRS := $(shell echo "-I"`find Hoi3ToDefcon/ -type d | tr "\\n" ":" | sed 's/:/ -I/g'`.)

SRC_FILES := $(shell find Hoi3ToDefcon/*.*)

CPP_FILES := $(shell find Hoi3ToDefcon/ -name '*.cpp')
OBJ_FILES := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))
DEP_FILES := $(addprefix depends/,$(notdir $(CPP_FILES:.cpp=.d)))

VPATH := $(shell find Hoi3ToDefcon/ -type d | tr "\\n" ":" )

# dependency smartness from http://scottmcpeak.com/autodepend/autodepend.html

all: exe

exe: Release/Hoi3ToDefconConverter

-include $(DEP_FILES)

Release/Hoi3ToDefconConverter: $(OBJ_FILES)
	g++ -o $@ $^ $(LD_FLAGS)

obj/%.o: %.cpp
	@mkdir -p obj
	@mkdir -p depends
	g++ $(CC_FLAGS) $(INCLUDE_DIRS) -c -o $@ $<
	@g++ -MM $(CC_FLAGS) $(INCLUDE_DIRS) -c $< > depends/$*.d
	@mv -f depends/$*.d depends/$*.d.tmp
	@sed -e 's|.*:|$@:|' < depends/$*.d.tmp > depends/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < depends/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> depends/$*.d
	@rm -f depends/$*.d.tmp
	
clean:
	@rm -rf obj
	@rm -rf depends
	@rm -f Release/Hoi3ToDefconConverter

run: all
	./Release/Hoi3ToDefconConverter
