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
**********

# Netronome notes (Lance)

To compile, do `make all_netronome`. To compile and run tests, do `make netronome_tests`.

I'm implementing the following instructions, roughly in order from top to bottom:

- `nop` :)

- `immed[dest, immed_data, shift_cntl]` (no opt_tok)
	- immed_data: 32-bit data where the upper 16 bits MUST be all 0s or 1s. 
	- shf_cntl: (optional - save this for later)

- `alu[dest, A_op, alu_op, B_op]` (no opt_tok or condition codes yet)
	1. `+ - B-A B`
	2. `+16 +8 ~B AND ~AND AND~ OR XOR`
	3. `+carry -carry` (involves previous carry out)

- `alu_shf[dest, A_op, alu_op, B_op, B_op_shf]` (no opt_tok or CCs yet)
	1. alu_ops: `B ~B AND ~AND AND~ OR`
	2. shift ops: `<<n >>n <<rotn >>rotn` (n in range 1..31)
	3. shift ops: `<<indirect >>indirect` (lower 5 bits of A operand of previous instruction)

- Other decisions/questions
	- some parts are case insensitive (which parts?)
	- Fixing input/output to be a particular GPR or transfer register
	- Check whether arithmetic is unsigned?
	- Check behavior of overflow while subtracting