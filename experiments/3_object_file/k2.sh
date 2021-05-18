#!/bin/bash
echo "Running K2 to optimize benchmark..."
benchmark=xdp1_kern
section=xdp1
if [ $# -gt 0 ]; then
  benchmark=$1
fi
if [ $# -gt 1 ]; then
  section=$2
fi
echo "Benchmark is" $benchmark $section
output_file=$benchmark.o.out
if [ ! -d "output" ]; then
  mkdir output
fi
python3 run_k2.py $benchmark.o $benchmark.desc $benchmark.k2_args --programs $section > run_k2_log.tmp
mv output/${section}_modified.o $output_file
echo "Finish running:"
grep "original" output/log.txt
echo "best program found by K2: "
grep "top 1 " output/log.txt
echo "Optimized program is stored in" $output_file
grep "compiling" output/log.txt
rm -rf output
rm -rf benchmark.insns
rm -rf run_k2_log.tmp

