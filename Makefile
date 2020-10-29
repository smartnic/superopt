include ../z3/build/config.mk
SRC=src/
VERIFY=src/verify/
SEARCH=src/search/
ISA=src/isa/
TOY_ISA=src/isa/toy-isa/
EBPF=src/isa/ebpf/
TOY_ISA_FLAG=-D ISA_TOY_ISA
EBPF_FLAG=-D ISA_EBPF

all: main.out main_ebpf.out z3server.out proposals_test.out inst_codegen_test_toy_isa.out inst_codegen_test_ebpf.out inst_test.out cost_test.out prog_test.out prog_test_ebpf.out mh_prog_test.out validator_test.out cfg_test.out inout_test.out smt_prog_test.out ebpf_inst_test.out validator_test_ebpf.out cfg_test_ebpf.out canonicalize_test_ebpf.out

main.out: main.cc main.h main_z3.o measure/benchmark_header.h measure/benchmark_toy_isa.cc measure/benchmark_toy_isa.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(TOY_ISA_FLAG) -std=c++11 main_z3.o measure/benchmark_toy_isa.cc measure/meas_mh_bhv.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc -o main.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

main_z3.o: main.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)main_z3.o  -I../z3/src/api -I../z3/src/api/c++ main.cc

main_ebpf.out: main.cc main.h main_ebpf_z3.o measure/benchmark_ebpf.cc measure/benchmark_ebpf.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(EBPF)canonicalize.h $(EBPF)canonicalize.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.cc $(EBPF)inst.h  $(EBPF)inst_var.cc $(EBPF)inst_var.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(EBPF_FLAG) -std=c++11 main_ebpf_z3.o measure/benchmark_ebpf.cc measure/meas_mh_bhv.cc $(ISA)inst.cc $(EBPF)inst.cc  $(EBPF)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(EBPF)inst_codegen.cc $(EBPF)canonicalize.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc  -o main_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

main_ebpf_z3.o: main.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)main_ebpf_z3.o  -I../z3/src/api -I../z3/src/api/c++ main.cc

z3server.out: z3server.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(CXX_OUT_FLAG)z3server.o -I../z3/src/api -I../z3/src/api/c++ z3server.cc
	$(CXX) z3server.o -o z3server.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

proposals_test.out: proposals_z3.o $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(SEARCH)proposals.cc $(ISA)prog.cc $(ISA)prog.h $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(TOY_ISA_FLAG) -std=c++11 $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SEARCH)proposals.cc $(SEARCH)proposals_z3.o $(ISA)prog.cc $(SRC)utils.cc $(ISA)inst_var.cc -o $(SEARCH)proposals_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

proposals_z3.o: $(SEARCH)proposals_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)proposals_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)proposals_test.cc

inst_codegen_test_toy_isa.out: inst_codegen_z3_toy_isa.o $(SRC)utils.cc $(SRC)utils.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(TOY_ISA_FLAG) -std=c++11 -fvisibility=hidden $(TOY_ISA)inst_codegen_z3.o $(TOY_ISA)inst_var.cc $(SRC)utils.cc $(ISA)inst.cc $(ISA)inst_var.cc -o $(TOY_ISA)inst_codegen_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

inst_codegen_z3_toy_isa.o: $(TOY_ISA)inst_codegen_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(TOY_ISA)inst_codegen_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(TOY_ISA)inst_codegen_test.cc

inst_codegen_test_ebpf.out: inst_codegen_z3_ebpf.o $(SRC)utils.cc $(SRC)utils.h $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(ISA)inst_var.cc $(ISA)inst_var.h $(EBPF)inst_var.h $(EBPF)inst_var.cc $(EBPF)bpf.h
	g++ $(EBPF_FLAG) -std=c++11 -fvisibility=hidden $(EBPF)inst_codegen_z3.o $(SRC)utils.cc $(EBPF)inst_codegen.cc $(ISA)inst.cc $(ISA)inst_var.cc $(EBPF)inst_var.cc -o $(EBPF)inst_codegen_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

inst_codegen_z3_ebpf.o: $(EBPF)inst_codegen_test.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(EBPF)inst_codegen_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(EBPF)inst_codegen_test.cc

inst_test.out: inst_z3.o $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(SRC)utils.cc $(SRC)utils.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(TOY_ISA_FLAG) -std=c++11 $(TOY_ISA)inst_z3.o $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SRC)utils.cc $(ISA)inst.cc $(ISA)inst_var.cc -o $(TOY_ISA)inst_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

inst_z3.o: $(TOY_ISA)inst_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(TOY_ISA)inst_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(TOY_ISA)inst_test.cc

ebpf_inst_test.out: ebpf_inst_z3.o $(EBPF)inst.cc $(EBPF)inst.h $(EBPF)bpf.h $(SRC)utils.cc $(SRC)utils.h $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(EBPF)inst_var.cc $(EBPF)inst_var.h $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(EBPF_FLAG) -std=c++11 -fvisibility=hidden $(EBPF)inst_z3.o $(EBPF)inst.cc $(SRC)utils.cc $(EBPF)inst_codegen.cc $(EBPF)inst_var.cc $(ISA)inst.cc $(ISA)inst_var.cc -o $(EBPF)inst_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

ebpf_inst_z3.o: $(EBPF)inst_test.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(EBPF)inst_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(EBPF)inst_test.cc

cost_test.out: $(SEARCH)cost.cc cost_z3.o $(SEARCH)cost.h $(SRC)inout.h $(SRC)inout.cc $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(ISA)prog.cc $(ISA)prog.h
	g++ $(TOY_ISA_FLAG) -std=c++11 $(SEARCH)cost.cc $(SEARCH)cost_z3.o $(SRC)inout.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(SRC)utils.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(ISA)prog.cc -o $(SEARCH)cost_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

cost_z3.o: $(SEARCH)cost_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)cost_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)cost_test.cc

prog_test.out: prog_z3.o $(ISA)prog.cc $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)prog.h $(SRC)utils.h $(SRC)utils.cc $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(TOY_ISA_FLAG) -std=c++11 $(ISA)prog_z3.o $(ISA)prog.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SRC)utils.cc $(ISA)inst_var.cc -o $(ISA)prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

prog_z3.o: $(ISA)prog_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(ISA)prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(ISA)prog_test.cc

prog_test_ebpf.out: prog_z3_ebpf.o $(ISA)prog.cc $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.h $(EBPF)inst.cc $(ISA)inst_var.h $(ISA)inst_var.cc $(EBPF)inst_var.h $(EBPF)inst_var.cc $(ISA)prog.h $(SRC)utils.h $(SRC)utils.cc
	g++ $(EBPF_FLAG) -std=c++11 $(ISA)prog_z3_ebpf.o $(ISA)prog.cc $(EBPF)inst_codegen.cc $(ISA)inst.cc $(EBPF)inst.cc $(ISA)inst_var.cc $(EBPF)inst_var.cc $(SRC)utils.cc -o $(ISA)prog_test_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

prog_z3_ebpf.o: $(ISA)prog_test_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(ISA)prog_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ $(ISA)prog_test_ebpf.cc

mh_prog_test.out: $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h mh_prog_z3.o $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc
	g++ $(TOY_ISA_FLAG) -std=c++11 $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(SEARCH)mh_prog_z3.o $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc measure/meas_mh_bhv.cc -o $(SEARCH)mh_prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

mh_prog_z3.o: $(SEARCH)mh_prog_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(SEARCH)mh_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(SEARCH)mh_prog_test.cc

validator_test.out: validator_z3.o $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)inout.cc $(SRC)inout.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(ISA)prog.cc $(ISA)prog.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h
	g++ $(TOY_ISA_FLAG) -std=c++11 -fvisibility=hidden $(VERIFY)validator_z3.o $(VERIFY)validator.cc $(VERIFY)z3client.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(SRC)inout.cc $(SRC)utils.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(ISA)prog.cc $(ISA)inst.cc -o $(VERIFY)validator_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

validator_z3.o: $(VERIFY)validator_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)validator_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)validator_test.cc

validator_test_ebpf.out: validator_z3_ebpf.o $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(EBPF)inst.cc $(EBPF)inst.h $(EBPF)inst_var.cc $(EBPF)inst_var.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)inout.cc $(SRC)inout.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(ISA)prog.cc $(ISA)prog.h $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(EBPF)canonicalize.cc $(EBPF)canonicalize.h $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h
	g++ $(EBPF_FLAG) -std=c++11 -fvisibility=hidden $(VERIFY)validator_z3_ebpf.o $(VERIFY)validator.cc $(VERIFY)z3client.cc $(EBPF)inst.cc $(EBPF)inst_var.cc $(VERIFY)cfg.cc $(SRC)inout.cc $(SRC)utils.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc  $(ISA)prog.cc $(EBPF)inst_codegen.cc $(EBPF)canonicalize.cc $(ISA)inst.cc -o $(VERIFY)validator_test_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

validator_z3_ebpf.o: $(VERIFY)validator_test_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)validator_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)validator_test_ebpf.cc

smt_prog_test.out: smt_prog_z3.o $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(TOY_ISA_FLAG) -std=c++11 -fvisibility=hidden $(VERIFY)smt_prog_z3.o $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(SRC)utils.cc -o $(VERIFY)smt_prog_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

smt_prog_z3.o: $(VERIFY)smt_prog_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)smt_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)smt_prog_test.cc

smt_prog_test_ebpf.out: smt_prog_z3_ebpf.o $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.cc $(EBPF)inst.h $(EBPF)inst_var.cc $(EBPF)inst_var.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(EBPF_FLAG) -std=c++11 -fvisibility=hidden $(VERIFY)smt_prog_z3_ebpf.o $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(EBPF)inst_codegen.cc $(ISA)inst.cc $(EBPF)inst.cc $(EBPF)inst_var.cc $(VERIFY)cfg.cc $(SRC)utils.cc -o $(VERIFY)smt_prog_test_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

smt_prog_z3_ebpf.o: $(VERIFY)smt_prog_test_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)smt_prog_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)smt_prog_test_ebpf.cc

cfg_test.out: cfg_z3.o $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(VERIFY)cfg.h $(VERIFY)cfg.cc $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(TOY_ISA_FLAG) -std=c++11 $(ISA)inst.cc $(TOY_ISA)inst.cc $(VERIFY)cfg.cc $(VERIFY)cfg_z3.o $(SRC)utils.cc $(ISA)inst_var.cc -o $(VERIFY)cfg_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

cfg_z3.o: $(VERIFY)cfg_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)cfg_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)cfg_test.cc

cfg_test_ebpf.out: cfg_z3_ebpf.o $(EBPF)inst_codegen.h $(EBPF)inst_codegen.cc $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.h $(EBPF)inst.cc $(EBPF)inst_var.h $(EBPF)inst_var.cc $(VERIFY)cfg.h $(VERIFY)cfg.cc $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ $(EBPF_FLAG) -std=c++11 $(EBPF)inst_codegen.cc $(ISA)inst.cc $(EBPF)inst.cc $(EBPF)inst_var.cc $(VERIFY)cfg.cc $(VERIFY)cfg_z3_ebpf.o $(SRC)utils.cc $(ISA)inst_var.cc -o $(VERIFY)cfg_test_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

cfg_z3_ebpf.o: $(VERIFY)cfg_test_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)cfg_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)cfg_test_ebpf.cc

inout_test.out: $(SRC)inout_test.cc $(SRC)inout.cc $(SRC)inout.h $(SRC)utils.cc $(SRC)utils.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc
	g++ $(TOY_ISA_FLAG) -std=c++11 $(SRC)inout_test.cc $(SRC)inout.cc $(SRC)utils.cc $(TOY_ISA)inst_var.cc -o $(SRC)inout_test.out

smt_var_test.out: smt_var_z3.o $(SRC)utils.cc $(SRC)utils.h $(ISA)inst_var.cc $(ISA)inst_var.h
	g++ -std=c++11 -fvisibility=hidden $(VERIFY)smt_var_z3.o $(SRC)utils.cc $(ISA)inst_var.cc -o $(VERIFY)smt_var_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

smt_var_z3.o: $(VERIFY)smt_var_test.cc
	$(CXX) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(VERIFY)smt_var_z3.o  -I../z3/src/api -I../z3/src/api/c++ $(VERIFY)smt_var_test.cc

canonicalize_test_ebpf.out: canonicalize_z3_ebpf.o $(EBPF)canonicalize.cc $(EBPF)canonicalize.h $(EBPF)inst.cc $(EBPF)inst.h $(EBPF)bpf.h $(SRC)utils.cc $(SRC)utils.h $(EBPF)inst_codegen.cc $(EBPF)inst_codegen.h $(EBPF)inst_var.cc $(EBPF)inst_var.h $(ISA)inst.cc $(ISA)inst.h $(ISA)inst_var.cc $(ISA)inst_var.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(ISA)inst_header_basic.h
	g++ $(EBPF_FLAG) -std=c++11 -fvisibility=hidden $(EBPF)canonicalize_z3_ebpf.o $(EBPF)canonicalize.cc $(EBPF)inst.cc $(SRC)utils.cc $(EBPF)inst_codegen.cc $(EBPF)inst_var.cc $(ISA)inst.cc $(ISA)inst_var.cc $(VERIFY)cfg.cc -o $(EBPF)canonicalize_test_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

canonicalize_z3_ebpf.o: $(EBPF)canonicalize_test.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)$(EBPF)canonicalize_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ $(EBPF)canonicalize_test.cc

clean:
	for i in */; do find . -name "*.o" -delete; done
	for i in */; do find . -name "*.out" -delete; done

run_tests:
	make
	./src/inout_test.out
	./src/isa/prog_test.out
	./src/isa/prog_test_ebpf.out
	./src/isa/toy-isa/inst_codegen_test.out
	./src/isa/toy-isa/inst_test.out
	./src/isa/ebpf/inst_codegen_test.out
	./src/isa/ebpf/inst_test.out
	./src/verify/validator_test.out
	./src/verify/validator_test_ebpf.out
	./src/verify/smt_prog_test.out
	./src/search/cost_test.out
	./src/verify/cfg_test.out
	./src/verify/cfg_test_ebpf.out
	./src/search/mh_prog_test.out
	./src/search/proposals_test.out
	./main.out
	./main_ebpf.out
	make all_measure
	./measure/meas_time.out
	./measure/meas_mh_bhv_test.out

all_measure: meas_time.out meas_mh_bhv_test.out

meas_time.out: measure/meas_time.cc measure/benchmark_toy_isa.cc measure/benchmark_toy_isa.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc meas_time_z3.o $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(TOY_ISA)inst.cc $(TOY_ISA)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(TOY_ISA_FLAG) -std=c++11 measure/meas_time_z3.o measure/benchmark_toy_isa.cc measure/meas_mh_bhv.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(TOY_ISA)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc -o measure/meas_time.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_time_z3.o: measure/meas_time.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_time_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_time.cc

meas_mh_bhv_test.out: meas_mh_bhv_z3.o measure/meas_mh_bhv.h measure/meas_mh_bhv.cc measure/benchmark_toy_isa.h measure/benchmark_toy_isa.cc $(ISA)prog.h $(ISA)prog.cc $(SRC)inout.h $(SRC)inout.cc $(TOY_ISA)inst_codegen.h $(TOY_ISA)inst_var.h $(TOY_ISA)inst_var.cc $(ISA)inst.cc  $(ISA)inst.h $(TOY_ISA)inst.h $(TOY_ISA)inst.cc $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(TOY_ISA_FLAG) -std=c++11 measure/meas_mh_bhv_z3.o measure/meas_mh_bhv.cc measure/benchmark_toy_isa.cc $(ISA)prog.cc $(SRC)inout.cc $(ISA)inst.cc $(TOY_ISA)inst.cc $(ISA)inst_var.cc $(SRC)utils.cc -o measure/meas_mh_bhv_test.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_mh_bhv_z3.o: measure/meas_mh_bhv_test.cc
	$(CXX) $(TOY_ISA_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_mh_bhv_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_mh_bhv_test.cc

meas_time_ebpf.out: measure/meas_time_ebpf.cc measure/benchmark_ebpf.cc measure/benchmark_ebpf.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc meas_time_z3_ebpf.o $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(EBPF)inst_codegen.h $(EBPF)inst_var.h $(EBPF)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.cc $(EBPF)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)z3client.cc $(VERIFY)z3client.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h
	g++ $(EBPF_FLAG) -std=c++11 measure/meas_time_z3_ebpf.o measure/benchmark_ebpf.cc measure/meas_mh_bhv.cc $(ISA)inst.cc $(EBPF)inst.cc $(EBPF)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(EBPF)inst_codegen.cc $(VERIFY)validator.cc $(VERIFY)z3client.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc -o measure/meas_time_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_time_z3_ebpf.o: measure/meas_time_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_time_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_time_ebpf.cc

meas_solve_time_ebpf.out: measure/meas_solve_time_ebpf.cc measure/benchmark_ebpf.cc measure/benchmark_ebpf.h measure/meas_mh_bhv.h measure/meas_mh_bhv.cc meas_solve_time_z3_ebpf.o $(SEARCH)mh_prog.cc $(SEARCH)mh_prog.h $(SEARCH)proposals.cc $(SEARCH)proposals.h $(ISA)prog.cc $(ISA)prog.h $(SEARCH)cost.cc $(SEARCH)cost.h $(SRC)inout.cc $(SRC)inout.h $(EBPF)inst_codegen.h $(EBPF)inst_var.h $(EBPF)inst_var.cc $(ISA)inst_header.h $(ISA)inst_header_basic.h $(ISA)inst.cc $(ISA)inst.h $(EBPF)inst.cc $(EBPF)inst.h $(VERIFY)validator.cc $(VERIFY)validator.h $(VERIFY)cfg.cc $(VERIFY)cfg.h $(VERIFY)smt_prog.cc $(VERIFY)smt_prog.h $(ISA)inst_var.cc $(ISA)inst_var.h $(SRC)utils.cc $(SRC)utils.h $(VERIFY)z3client.cc $(VERIFY)z3client.h
	g++ $(EBPF_FLAG) -std=c++11 measure/meas_solve_time_z3_ebpf.o measure/benchmark_ebpf.cc measure/meas_mh_bhv.cc $(ISA)inst.cc $(EBPF)inst.cc $(EBPF)inst_var.cc $(SEARCH)mh_prog.cc $(SEARCH)proposals.cc $(ISA)prog.cc $(SEARCH)cost.cc $(SRC)inout.cc $(EBPF)inst_codegen.cc $(VERIFY)validator.cc $(VERIFY)cfg.cc $(VERIFY)smt_prog.cc $(ISA)inst_var.cc $(SRC)utils.cc $(VERIFY)z3client.cc -o measure/meas_solve_time_ebpf.out ../z3/build/libz3$(SO_EXT) $(LINK_EXTRA_FLAGS)

meas_solve_time_z3_ebpf.o: measure/meas_solve_time_ebpf.cc
	$(CXX) $(EBPF_FLAG) $(CXXFLAGS) $(OS_DEFINES) $(EXAMP_DEBUG_FLAG) $(CXX_OUT_FLAG)measure/meas_solve_time_z3_ebpf.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_solve_time_ebpf.cc