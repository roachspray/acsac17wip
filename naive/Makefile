LLBIN=/usr/lib/llvm-4.0/bin
LLVM_CONFIG=$(LLBIN)/llvm-config
#QUIET:=@
QUIET:=

SRC_DIR?=$(PWD)/src
#3.8 flags
LDFLAGS38=-lpthread -ldl -lz -lcurses
LDFLAGS+=$(shell $(LLVM_CONFIG) --ldflags) $(LDFLAGS38) 

COMMON_FLAGS=-Wall -Wextra -g

# For PSKIN_ROOT
PR=.

CXXFLAGS+=$(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags)
CPPFLAGS+=$(shell $(LLVM_CONFIG) --cppflags) -std=c++11 -I$(SRC_DIR) 

LOADABLE_MODULE_OPTIONS=-shared
#LOADABLE_MODULE_OPTIONS=-shared -Wl,-O1

LDIS=$(LLBIN)/llvm-dis
CPP=$(LLBIN)/clang++
CC=$(LLBIN)/clang
LLC=$(LLBIN)/llc
OPTB=$(LLBIN)/opt

BOD=build/obj
PASSMGR=Pruneskin
OPM=build/bin/$(PASSMGR)

PASS=Pruneskin.so

TRACEPASS=TraceInject.so
TRACEPASS_OBJECTS=Transform/TraceBBInject.o  \
 Transform/LabelMDInject.o  \
 Transform/UnlabelMDInjected.o  \
 TraceUtils.o 

GBPASS=GraphBlock.so
GBPASS_OBJECTS=Analysis/GraphBlock.o  \
 GVizUtils.o


BBCHAINPASS=NoPredBlocks.so
BBCHAINPASS_OBJECTS=Analysis/NoPredBlocks.o

NPPASS=NaivePrune.so
NPPASS_OBJECTS=Transform/PruneBlocks.o  \
 Transform/PruneHelp.o  \
 CommonUtils.o  \
 TraceUtils.o 

GVPASS=GVizPrune.so
GVPASS_OBJECTS=Transform/GVizPruneBlocks.o  \
 Transform/PruneHelp.o  \
 CommonUtils.o \
 GVizUtils.o  \
 TraceUtils.o

GVDPASS=GVizTraceDiff.so
GVDPASS_OBJECTS=Analysis/GVizTraceDiff.o  \
 TraceUtils.o  \
 GVizUtils.o


PASS_OBJECTS=Transform/TraceBBInject.o  \
 TraceUtils.o  \
 GVizUtils.o  \
 Transform/PruneHelp.o  \
 Transform/PruneBlocks.o  \
 Transform/GVizPruneBlocks.o  \
 Transform/ProbPruneBlocks.o  \
 Transform/PrPrune.o  \
 Analysis/GraphBlock.o  \
 Analysis/GVizTraceDiff.o  \
 Analysis/CycloComplexity.o  \
 Analysis/DumpUsersPass.o 


LIBS=$(shell $(LLVM_CONFIG) --libs) 

TDIR=build/tests

#default: prep $(TRACEPASS) $(GBPASS) $(NPPASS) $(GVPASS) $(GVDPASS)

default: prep $(TRACEPASS) $(NPPASS) $(BBCHAINPASS)  $(GVDPASS)


#passmgr

prep:
	@echo "Prep phase"
	$(QUIET)mkdir -p build/obj
	$(QUIET)mkdir -p build/obj/Analysis
	$(QUIET)mkdir -p build/obj/Transform
	$(QUIET)mkdir -p build/bin
	$(QUIET)mkdir -p build/lib

define builditdood
$(QUIET)$(CPP) -o $(BOD)/$(1)/$(@F) -c $(CPPFLAGS) $(CXXFLAGS) $<
endef

Transform/%.o: $(SRC_DIR)/Transform/%.cpp
	@echo "Compiling $*.cpp"
	$(call builditdood,Transform)

Analysis/%.o: $(SRC_DIR)/Analysis/%.cpp
	@echo "Compiling $*.cpp"
	$(call builditdood,Analysis)

%.o : $(SRC_DIR)/%.cpp
	@echo "Compiling $*.cpp"
	$(call builditdood,.)

passmgr:
	@echo "Building passmanager clean up ldflags XXX"
	$(QUIET)$(CPP) -o $(BOD)/Pruneskin.o -c $(CPPFLAGS) $(CXXFLAGS) src/Pruneskin.cpp
	$(QUIET)$(CPP) -o $(OPM) $(CXXFLAGS) build/obj/Pruneskin.o ${addprefix $(BOD)/,$(PASS_OBJECTS)} $(LDFLAGS) $(LIBS)

$(TRACEPASS) : $(TRACEPASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

$(NPPASS) : $(NPPASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

$(BBCHAINPASS) : $(BBCHAINPASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}


$(GBPASS) : $(GBPASS_OBJECTSS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

$(GVPASS) : $(GVPASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

$(GVDPASS) : $(GVDPASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

$(PASS) : $(PASS_OBJECTS)
	@echo "Linking $@"
	$(QUIET)$(CPP) -o build/lib/$@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) ${addprefix $(BOD)/,$^}

clean:
	$(QUIET)rm -rf build Pruneskin

libs:
	cd libtrays && make -f Makefile	

cleanlibs:
	cd libtrays && make -f Makefile	clean

cleanall: clean cleanlibs
