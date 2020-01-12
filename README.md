Superopt 

#### Installation: Linux (Ubuntu 18.04) and macOS (10.15.2)

* Install `z3`, more in https://github.com/Z3Prover/z3
```
git clone https://github.com/Z3Prover/z3.git
cd z3
git checkout 4643fdaa4e909d99e5888e4a6574d066c7516482
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
