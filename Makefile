all: proposals_test.out inst_test.out cost_test.out prog_test.out mh_prog_test.out validator_test.out cfg_test.out inout_test.out smt_prog_test.out

proposals_test.out: inst.cc inst.h proposals.cc proposals_test.cc prog.cc prog.h utils.cc utils.h
	g++ -std=c++11 inst.cc proposals.cc proposals_test.cc prog.cc utils.cc -o proposals_test.out

inst_test.out: inst.cc inst.h inst_test.cc utils.cc utils.h
	g++ -std=c++11 inst.cc inst_test.cc utils.cc -o inst_test.out

cost_test.out: cost.cc cost_z3.o cost.h inout.h inout.cc inst.cc inst.h validator.cc validator.h cfg.cc cfg.h utils.cc utils.h smt_prog.cc smt_prog.h prog.cc prog.h
	g++ -std=c++11 cost.cc cost_z3.o inout.cc inst.cc validator.cc cfg.cc utils.cc smt_prog.cc prog.cc -o cost_test.out ../z3/build/libz3.dylib -lpthread

cost_z3.o: cost_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o cost_z3.o  -I../z3/src/api -I../z3/src/api/c++ cost_test.cc

prog_test.out: prog.cc inst.h inst.cc prog.h prog_test.cc utils.h utils.cc
	g++ -std=c++11 prog_test.cc prog.cc inst.cc utils.cc -o prog_test.out

mh_prog_test.out: mh_prog.cc mh_prog.h mh_prog_z3.o proposals.cc proposals.h prog.cc prog.h cost.cc cost.h inout.cc inout.h inst.cc inst.h validator.cc validator.h cfg.cc cfg.h smt_prog.cc smt_prog.h utils.cc utils.h measure/meas_mh_data.h measure/meas_mh_data.cc
	g++ -std=c++11 inst.cc mh_prog.cc proposals.cc prog.cc cost.cc inout.cc validator.cc cfg.cc mh_prog_z3.o smt_prog.cc utils.cc measure/meas_mh_data.cc -o mh_prog_test.out ../z3/build/libz3.dylib -lpthread

mh_prog_z3.o: mh_prog_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o mh_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ mh_prog_test.cc

validator_test.out: validator_z3.o validator.cc validator.h inst.cc inst.h cfg.cc cfg.h inout.cc inout.h utils.cc utils.h smt_prog.cc smt_prog.h
	g++ -std=c++11 validator_z3.o validator.cc inst.cc cfg.cc inout.cc utils.cc smt_prog.cc -o validator_test.out ../z3/build/libz3.dylib -lpthread

validator_z3.o: validator_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o validator_z3.o  -I../z3/src/api -I../z3/src/api/c++ validator_test.cc

smt_prog_test.out: smt_prog_z3.o smt_prog.cc smt_prog.h inst.cc inst.h cfg.cc cfg.h utils.cc utils.h
	g++ -std=c++11 smt_prog_z3.o smt_prog.cc inst.cc cfg.cc utils.cc -o smt_prog_test.out ../z3/build/libz3.dylib -lpthread

smt_prog_z3.o: smt_prog_test.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o smt_prog_z3.o  -I../z3/src/api -I../z3/src/api/c++ smt_prog_test.cc

cfg_test.out: inst.h inst.cc cfg.h cfg.cc cfg_test.cc
	g++ -std=c++11 inst.cc cfg.cc cfg_test.cc -o cfg_test.out

inout_test.out: inout_test.cc inout.cc inout.h utils.cc utils.h
	g++ -std=c++11 inout_test.cc inout.cc utils.cc -o inout_test.out

clean:
	rm -f proposals_test.out inst_test.out cost_test.out cost_z3.o prog_test.out mh_prog_test.out mh_prog_z3.o validator_test.out validator_z3.o cfg_test.out inout_test.out smt_prog_test.out smt_prog_z3.o

all_measure: meas_time.out meas_mh_bhv.out meas_mh_data_test.out

meas_time.out: measure/meas_time.cc measure/common.cc measure/common.h measure/meas_mh_data.h measure/meas_mh_data.cc meas_time_z3.o mh_prog.cc mh_prog.h proposals.cc proposals.h prog.cc prog.h cost.cc cost.h inout.cc inout.h inst.cc inst.h validator.cc validator.h cfg.cc cfg.h smt_prog.cc smt_prog.h utils.cc utils.h
	g++ -std=c++11 measure/meas_time_z3.o measure/common.cc measure/meas_mh_data.cc inst.cc mh_prog.cc proposals.cc prog.cc cost.cc inout.cc validator.cc cfg.cc smt_prog.cc utils.cc -o measure/meas_time.out ../z3/build/libz3.dylib -lpthread

meas_time_z3.o: measure/meas_time.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o measure/meas_time_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_time.cc

meas_mh_bhv.out: measure/meas_mh_bhv.cc measure/common.cc measure/common.h meas_mh_bhv_z3.o measure/meas_mh_data.h measure/meas_mh_data.cc mh_prog.cc mh_prog.h proposals.cc proposals.h prog.cc prog.h cost.cc cost.h inout.cc inout.h inst.cc inst.h validator.cc validator.h cfg.cc cfg.h smt_prog.cc smt_prog.h utils.cc utils.h
	g++ -std=c++11 measure/meas_mh_bhv_z3.o measure/common.cc measure/meas_mh_data.cc inst.cc mh_prog.cc proposals.cc prog.cc cost.cc inout.cc validator.cc cfg.cc smt_prog.cc utils.cc -o measure/meas_mh_bhv.out ../z3/build/libz3.dylib -lpthread

meas_mh_bhv_z3.o: measure/meas_mh_bhv.cc
	g++ -D_MP_INTERNAL -DNDEBUG -D_EXTERNAL_RELEASE -std=c++11 -fvisibility=hidden -c -mfpmath=sse -msse -msse2 -O3 -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-unused-value -fPIC -o measure/meas_mh_bhv_z3.o  -I../z3/src/api -I../z3/src/api/c++ measure/meas_mh_bhv.cc

meas_mh_data_test.out: measure/meas_mh_data_test.cc measure/meas_mh_data.h measure/meas_mh_data.cc measure/common.h measure/common.cc prog.h prog.cc inout.h inout.cc inst.h inst.cc
	g++ -std=c++11 measure/meas_mh_data_test.cc measure/meas_mh_data.cc measure/common.cc prog.cc inout.cc inst.cc -o measure/meas_mh_data_test.out

path_measure=measure/
clean_measure:
	rm -f ${path_measure}meas_time.out ${path_measure}meas_time_z3.o ${path_measure}meas_mh_bhv.out ${path_measure}meas_mh_bhv_z3.o ${path_measure}meas_mh_data_test.out
