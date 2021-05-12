#!/bin/bash
path_prefix=experiments/2_change_benchmark/2_bpf_insn/
benchmark=benchmark_before.bpf_insn
if [ $# -gt 0 ]; then
  benchmark=$1
fi
echo "Benchmark is" $benchmark
output_file=$benchmark.out
if [ ! -d "output" ]; then
  mkdir output
fi
cd ../../../
cp input_translater/bpf_insn_prog.h input_translater/bpf_insn_prog_bk.h
cp $path_prefix$benchmark input_translater/bpf_insn_prog.h
make bpf_insn_translater.out > compile_bpf_insn_translater.tmp_log
rm -rf compile_bpf_insn_translater.tmp_log
./input_translater/bpf_insn_translater.out experiments/2_change_benchmark/2_bpf_insn/benchmark.insns
echo "Running K2 to optimize benchmark..."
./main_ebpf.out --bm_from_file --desc experiments/2_change_benchmark/2_bpf_insn/benchmark.desc --bytecode experiments/2_change_benchmark/2_bpf_insn/benchmark.insns --map experiments/2_change_benchmark/2_bpf_insn/benchmark.maps -k 1 --is_win --port 8000 --logger_level 1 --w_e 0.5 --w_p 1.5 --st_ex 0 --st_eq 0 --st_avg 1 --st_perf 0 --st_when_to_restart 0 --st_when_to_restart_niter 0 --st_start_prog 0 --p_inst_operand 0.33333333 --p_inst 0.33333333 --p_inst_as_nop 0.15 --reset_win_niter 5000 --win_s_list 0 --win_e_list 3 --path_res experiments/2_change_benchmark/2_bpf_insn/output/ -n 50000 > experiments/2_change_benchmark/2_bpf_insn/output/log.txt
cd experiments/2_change_benchmark/2_bpf_insn
echo "Finish running:"
grep "original" output/log.txt
echo "best program found by K2: "
grep "top 1 " output/log.txt
echo "Optimized program is stored in" $output_file
mv output/output0.desc $output_file
grep "compiling" output/log.txt
rm -rf output
rm -rf benchmark.insns
mv ../../../input_translater/bpf_insn_prog_bk.h ../../../input_translater/bpf_insn_prog.h
