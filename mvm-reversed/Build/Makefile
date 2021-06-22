CXXFLAGS:=-m32 -std=gnu++14 -Og -g -Wall -fdiagnostics-color=always -Wno-unused-variable -Wno-unused-function -include all.h -Wno-unknown-pragmas -Wno-unused-local-typedefs -Wno-reorder -fno-diagnostics-show-caret -fvisibility=hidden -Wno-sign-compare -Wno-narrowing
LDFLAGS :=
DEFINES:= \
	-DPOSIX=1 \
	-DGNUC=1 \
	-D_LINUX=1 \
	-DRAD_TELEMETRY_DISABLED=1 \
	-DGAME_DLL=1
INCLUDES:= \
	-I..
	-I/pool/Game/TF2/source-sdk-2013/mp/src \
	-I/pool/Game/TF2/source-sdk-2013/mp/src/public \
	-I/pool/Game/TF2/source-sdk-2013/mp/src/public/tier0 \
	-I/pool/Game/TF2/source-sdk-2013/mp/src/public/tier1 \
	-I/pool/Game/TF2/source-sdk-2013/mp/src/game/shared \
	-I/pool/Game/TF2/source-sdk-2013/mp/src/game/server

SRC_DIRS:= \
	../CTFBot \
	../Entity \
	../NextBot \
	../Path \
	../Population

SOURCES_CXX:=$(shell find $(SRC_DIRS) -follow -type f -name '*.cpp')
OBJECTS:=$(patsubst %.cpp,%.o,$(SOURCES_CXX))
HEADERS:=$(shell find . -follow -type f -name '*.h')


.PHONY: all clean
all: mvm
clean:
	rm -f mvm $(shell find . -iname '*.o')

mvm: $(OBJECTS) Makefile
	g++ $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.cpp $(HEADERS) Makefile
	g++ $(CXXFLAGS) $(INCLUDES) $(DEFINES) -o $@ -c $<
