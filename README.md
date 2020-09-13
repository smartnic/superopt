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
git clone https://github.com/ngsrinivas/superopt.git
cd superopt
make main.out
```
