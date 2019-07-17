all: proposals_test.out inst_test.out

proposals_test.out: inst.cc inst.h proposals.cc proposals_test.cc
	g++ -std=c++11 inst.cc proposals.cc proposals_test.cc -o proposals_test.out

inst_test.out: inst.cc inst.h inst_test.cc
	g++ -std=c++11 inst.cc inst_test.cc -o inst_test.out

clean:
	rm -f proposals_test.out inst_test.out
