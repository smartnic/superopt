echo "Compiling K2..."
cp benchmark.h ../../measure/benchmark.h
mkdir output
cd ../../
make main_ebpf.out
make z3server.out
echo "Running K2..."
./main_ebpf.out  --bm 0 -k 1 --is_win --port 8000 --logger_level 1 --w_e 0.5 --w_p 1.5 --st_ex 0 --st_eq 0 --st_avg 1 --st_perf 0 --st_when_to_restart 0 --st_when_to_restart_niter 0 --st_start_prog 0 --p_inst_operand 0.33333333 --p_inst 0.33333333 --p_inst_as_nop 0.15 --reset_win_niter 5000 --win_s_list 0 --win_e_list 1 --path_res experiments/1_hello_world/output/ -n 50000 > experiments/1_hello_world/output/log.txt
make clean
cd experiments/1_hello_world
echo "Finish running. Output file: output/output0.desc"
