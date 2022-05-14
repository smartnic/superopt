Superopt 

#### Installation: Linux (Ubuntu 18.04) and macOS (10.15.2)

* Install `z3`, more in https://github.com/Z3Prover/z3
```
git clone https://github.com/Z3Prover/z3.git
cd z3
git checkout 1c7d27bdf31ca038f7beee28c41aa7dbba1407dd
python scripts/mk_make.py
cd build
make
sudo make install
```
* Install `superopt`. Keep superopt folder and z3 folder in the same directory level
```
cd ../../
git clone https://github.com/smartnic/superopt.git
cd superopt
make main_ebpf.out
```

Example command to run k2 for repair:

./main_ebpf.out --bm 26 --functionality 1 --w_e 0 --w_p 0 --w_s 2 -k 1 --port 8000 --logger_level 1 --p_inst_operand 0.33333333 --p_inst 0.33333333 --p_inst_as_nop 0.15 --is_win --win_s_list 0 --win_e_list 1 -n 1000 --reset_win_niter 800 --path_res output/

Todo: add more instructions soon
