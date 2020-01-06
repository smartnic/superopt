include ../z3/build/config.mk
SRC=src/
VERIFY=src/verify/
SEARCH=src/search/
ISA=src/isa/
TOY_ISA=src/isa/toy-isa/

all: main.out proposals_test.out inst_codegen_test.out inst_test.out cost_test.out prog_test.out mh_prog_test.out validator_test.out cfg_test.out inout_test.out smt_prog_test.out

main.out: main.cc main.h main_z3.o measure/common.cc measure/common.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(TOY_ISA)prog.cc $(TOY_ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ -std=c++11 main_z3.o measure/common.cc measure/meas_mh_bhv.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(TOY_ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(SRC)utils.cc -o main.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

main_z3.o: main.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)main_z3.o  -I../z3/src/api -I../z3/src/api/c++ main.cc

proposals_test.out: proposals_z3.o $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(SEARCH)proposals.cc $(TOY_ISA)prog.cc $(TOY_ISA)prog.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h
	g++ -std=c++11 $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(SEARCH)proposals.cc $(SEARCH)proposals_z3.o $(TOY_ISA)prog.cc $(SRC)utils.cc $(VERIFY)smt_var.cc -o $(SEARCH)proposals_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

proposals_z3.o: $(SEARCH)proposals_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)proposals_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)proposals_test.cc

inst_codegen_test.out: inst_codegen_z3.o $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h
	g++ -std=c++11 -fvisibility=hidden $(ISA)inst_codegen_z3.o $(SRC)utils.cc $(ISA)inst_codegen.cc $(ISA)inst.cc -o $(ISA)inst_codegen_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

inst_codegen_z3.o: $(ISA)inst_codegen_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(ISA)inst_codegen_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(ISA)inst_codegen_test.cc

inst_test.out: inst_z3.o $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h
	g++ -std=c++11 $(TOY_ISA)inst_z3.o $(TOY_ISA)inst.cc $(SRC)utils.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(VERIFY)smt_var.cc -o $(TOY_ISA)inst_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

inst_z3.o: $(TOY_ISA)inst_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(TOY_ISA)inst_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(TOY_ISA)inst_test.cc

cost_test.out: $(SEARCH)cost.cc cost_z3.o $(SEARCH)cost.h $(SRC)inout.h $(SRC)inout.cc $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(TOY_ISA)prog.cc $(TOY_ISA)prog.h
	g++ -std=c++11 $(SEARCH)cost.cc $(SEARCH)cost_z3.o $(SRC)inout.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)validator.cc $(VERIFY)cfg.cc $(SRC)utils.cc $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(TOY_ISA)prog.cc -o $(SEARCH)cost_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

cost_z3.o: $(SEARCH)cost_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)cost_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)cost_test.cc

prog_test.out: prog_z3.o $(TOY_ISA)prog.cc $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)prog.h $(SRC)utils.h $(SRC)utils.cc $(VERIFY)smt_var.cc $(VERIFY)smt_var.h
	g++ -std=c++11 $(TOY_ISA)prog_z3.o $(TOY_ISA)prog.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(SRC)utils.cc $(VERIFY)smt_var.cc -o $(TOY_ISA)prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

prog_z3.o: $(TOY_ISA)prog_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(TOY_ISA)prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(TOY_ISA)prog_test.cc

mh_prog_test.out: $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h mh_prog_z3.o $(SEARCH)proposals.cc $(SEARCH)proposals.h $(TOY_ISA)prog.cc $(TOY_ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(SRC)utils.cc $(SRC)utils.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc
	g++ -std=c++11 $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(TOY_ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)cfg.cc $(SEARCH)mh_prog_z3.o $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(SRC)utils.cc measure/meas_mh_bhv.cc -o $(SEARCH)mh_prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

mh_prog_z3.o: $(SEARCH)mh_prog_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)mh_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)mh_prog_test.cc

validator_test.out: validator_z3.o $(VERIFY)validator.cc $(VERIFY)validator.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)inout.cc $(SRC)inout.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h
	g++ -std=c++11 -fvisibility=hidden $(VERIFY)validator_z3.o $(VERIFY)validator.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(SRC)inout.cc $(SRC)utils.cc $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(ISA)inst_codegen.cc $(ISA)inst.cc -o $(VERIFY)validator_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

validator_z3.o: $(VERIFY)validator_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)validator_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)validator_test.cc

smt_prog_test.out: smt_prog_z3.o $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)utils.cc $(SRC)utils.h
	g++ -std=c++11 -fvisibility=hidden $(VERIFY)smt_prog_z3.o $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(SRC)utils.cc -o $(VERIFY)smt_prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

smt_prog_z3.o: $(VERIFY)smt_prog_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)smt_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)smt_prog_test.cc

cfg_test.out: cfg_z3.o $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(VERIFY)cfg.h $(VERIFY)cfg.cc $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h
	g++ -std=c++11 $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(VERIFY)cfg_z3.o $(SRC)utils.cc $(VERIFY)smt_var.cc -o $(VERIFY)cfg_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

cfg_z3.o: $(VERIFY)cfg_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)cfg_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)cfg_test.cc

inout_test.out: $(SRC)inout_test.cc $(SRC)inout.cc $(SRC)inout.h $(SRC)utils.cc $(SRC)utils.h
	g++ -std=c++11 $(SRC)inout_test.cc $(SRC)inout.cc $(SRC)utils.cc -o $(SRC)inout_test.out

clean:
	for i in */; do find . -name "*.o" -delete; done
	for i in */; do find . -name "*.out" -delete; done

run_tests:
	make
	./src/inout_test.out
	./src/isa/inst_codegen_test.out
	./src/isa/toy-isa/prog_test.out
	./src/isa/toy-isa/inst_test.out
	./src/verify/validator_test.out
	./src/verify/smt_prog_test.out
	./src/search/cost_test.out
	./src/verify/cfg_test.out
	./src/search/mh_prog_test.out
	./src/search/proposals_test.out
	./main.out
	make all_measure
	./measure/meas_time.out
	./measure/meas_mh_bhv_test.out

all_measure: meas_time.out meas_mh_bhv_test.out

meas_time.out: measure/meas_time.cc measure/common.cc measure/common.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc meas_time_z3.o $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(TOY_ISA)prog.cc $(TOY_ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ -std=c++11 measure/meas_time_z3.o measure/common.cc measure/meas_mh_bhv.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(TOY_ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(VERIFY)smt_var.cc $(SRC)utils.cc -o measure/meas_time.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_time_z3.o: measure/meas_time.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_time_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_time.cc

meas_mh_bhv_test.out: meas_mh_bhv_z3.o measure/meas_mh_bhv.h measure/meas_mh_bhv.cc measure/common.h measure/common.cc $(TOY_ISA)prog.h $(TOY_ISA)prog.cc $(SRC)inout.h $(SRC)inout.cc $(ISA)inst_codegen.cc $(ISA)inst_codegen.h $(ISA)inst.cc  $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(VERIFY)smt_var.cc $(VERIFY)smt_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ -std=c++11 measure/meas_mh_bhv_z3.o measure/meas_mh_bhv.cc measure/common.cc $(TOY_ISA)prog.cc $(SRC)inout.cc $(ISA)inst_codegen.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)smt_var.cc $(SRC)utils.cc -o measure/meas_mh_bhv_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_mh_bhv_z3.o: measure/meas_mh_bhv_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_mh_bhv_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_mh_bhv_test.cc
