#!/bin/bash
path_prefix=experiments/2_change_benchmark/
benchmark=benchmark_before.k2_inst
if [[ $# -gt 0 ]]; then
  benchmark=$1
fi
echo "Benchmark is" $benchmark
output_file=$benchmark.out
if [ ! -d "output" ]; then
  mkdir output
fi
cd ../../
echo "Running K2 to optimize benchmark..."
./input_translater/k2_inst_translater.out $path_prefix$benchmark experiments/2_change_benchmark/benchmark.insns
./main_ebpf.out --bm_from_file --desc experiments/2_change_benchmark/benchmark.desc --bytecode experiments/2_change_benchmark/benchmark.insns --map experiments/2_change_benchmark/benchmark.maps -k 1 --is_win --port 8000 --logger_level 1 --w_e 0.5 --w_p 1.5 --st_ex 0 --st_eq 0 --st_avg 1 --st_perf 0 --st_when_to_restart 0 --st_when_to_restart_niter 0 --st_start_prog 0 --p_inst_operand 0.33333333 --p_inst 0.33333333 --p_inst_as_nop 0.15 --reset_win_niter 5000 --win_s_list 0 --win_e_list 3 --path_res experiments/2_change_benchmark/output/ -n 50000 > experiments/2_change_benchmark/output/log.txt
cd experiments/2_change_benchmark
echo "Finish running:"
grep "original" output/log.txt
echo "best program found by K2: "
grep "top 1 " output/log.txt
echo "Optimized program is stored in" $output_file
mv output/output0.desc $output_file
grep "compiling" output/log.txt
rm -rf output
rm -rf benchmark.insns
