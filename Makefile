all: proposals_test.out inst_test.out cost_test.out prog_test.out mh_prog_test.out validator_test.out cfg_test.out inout_test.out

proposals_test.out: inst.cc inst.h proposals.cc proposals_test.cc prog.cc prog.h
	g++ -std=c++11 inst.cc proposals.cc proposals_test.cc prog.cc -o proposals_test.out

inst_test.out: inst.cc inst.h inst_test.cc
	g++ -std=c++11 inst.cc inst_test.cc -o inst_test.out

cost_test.out: cost.cc cost_z3.o cost.h inout.h inout.cc inst.cc inst.h validator.cc validator.h cfg.cc cfg.h utils.cc utils.h
	g++ -std=c++11 cost.cc cost_z3.o inout.cc inst.cc validator.cc cfg.cc utils.cc -o cost_test.out ../z3/build/libz3.dylib -lpthread

cost_z3.o: cost_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o cost_z3.o  -I../z3/src/api -I../z3/src/api/c++ cost_test.cc

prog_test.out: prog.cc inst.h inst.cc prog.h prog_test.cc
	g++ -std=c++11 prog_test.cc prog.cc inst.cc -o prog_test.out

mh_prog_test.out: mh_prog.cc mh_prog.h mh_prog_test.cc proposals.cc proposals.h prog.cc prog.h cost.cc cost.h inout.cc inout.h inst.cc inst.h validator.cc validator.h cfg.cc cfg.h mh_prog_z3.o
	g++ -std=c++11 inst.cc mh_prog.cc proposals.cc prog.cc cost.cc inout.cc validator.cc cfg.cc mh_prog_z3.o -o mh_prog_test.out ../z3/build/libz3.dylib -lpthread

mh_prog_z3.o: mh_prog_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o mh_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ mh_prog_test.cc

validator_test.out: validator_z3.o validator.cc inst.cc cfg.h cfg.cc inout.cc utils.cc utils.h
	g++ -std=c++11 validator_z3.o validator.cc inst.cc cfg.cc inout.cc utils.cc -o validator_test.out ../z3/build/libz3.dylib -lpthread

validator_z3.o:
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o validator_z3.o  -I../z3/src/api -I../z3/src/api/c++ ../superopt/validator_test.cc

cfg_test.out: inst.h inst.cc cfg.h cfg.cc cfg_test.cc
	g++ -std=c++11 inst.cc cfg.cc cfg_test.cc -o cfg_test.out

inout_test.out: inout_test.cc inout.cc inout.h utils.cc utils.h
	g++ -std=c++11 inout_test.cc inout.cc utils.cc -o inout_test.out

clean:
	rm -f proposals_test.out inst_test.out cost_test.out cost_z3.o prog_test.out mh_prog_test.out mh_prog_z3.o validator_test.out validator_z3.o cfg_test.out inout_test.out
