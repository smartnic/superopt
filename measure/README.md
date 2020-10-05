Measuring the solving time of equivalence check formula in the validator

##### How to use it
* make
```
make z3server.out; make meas_solve_time_ebpf.out;
```

* use the tool to get the solving time
```
./measure/meas_solve_time_ebpf.out [loop_times]
```
`loop_times` is the repeat times of running the tool for one benchmark pair. The default value of `loop_times` is 1

* how to read the output
```
./measure/meas_solve_time_ebpf.out 2
```
output:
```
Original program is rcv-sock4
starting p1
validator is_smt_valid: 9.51441e+07 us 1
validator is_smt_valid: 6.94754e+07 us 1
starting p2
validator is_smt_valid: 8.34699e+07 us 1
validator is_smt_valid: 9.16541e+07 us 1
```
Here two benchmark pairs are (rcv-sock4, p1) and (rcv-sock4, p2). The measurement was repeated twice. The first and second solving times of checking whether rcv-sock4 and p1 are equal is 9.51441e+07 and 6.94754e+07 us respectively.

* clean the environment
```
pkill z3server.out
```
