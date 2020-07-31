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

# Netronome notes (7/31/2020)

These notes document the current implementation of the Netronome ISA within the superopt framework, as of the end of Lance's DIMACS REU program in summer 2020. 

I list the different unit tests in order from lowest to highest level, which is the order that I worked on them. To compile all the tests, do `make all_netronome`. To compile and run tests, do `make netronome_tests`. (Both of these exclude the tests for parsing an input Netronome .list file, because that isn't complete yet.)


## Interpretation

The relevant test cases are in `src/isa/netronome/inst_test.cc`, which tests the function `interpret()` defined in `src/isa/netronome/inst.h`. In terms of program interpration, I've implemented the following instructions:

- `nop` :)

- `immed[dest, immed_data]` 
	- I'm currently treating immed_data as a literal 32-bit unsigned value. In fact, Netronome imposes a restriction where the upper upper 16 bits MUST be all 0s or 1s--I don't check for this yet.
	- There's a 3rd argument `shf_cntl` which is optional and isn't implemented yet.
	- No opt_tok yet

- `alu[dest, A_op, alu_op, B_op]`
    - I've implemnted all 14 alu_op types except for the operation `-carry`. 
    - The op types `+carry` and `-carry` require the carry out of the previous ALU operation to be saved in program state. I've implemented that as the property `int _unsigned_carry` in `class prog_state`.
    - Both `A_op` and `B_op` need to be registers for now. Netronome also these arguments to be constant numbers--this isn't implemented yet. In order to do so, the class `inst`, which models a single instruction, needs a way to remember whether these two arguments represent numbers or indices to a register.
    - No opt_tok or condition codes have been implemented yet

- Currently, I'm treating the initial value of the first register (register _a0_) as program input, and its final value as program output. 

## Instruction-level formalization

The relevant tests are in `src/isa/netronome/inst_codegen_tests.cc`, which test, for each operation, that its z3 predicate implementation matches its concrete computation (the `predicate_*` and `compute_*` functions defined at the top of `src/isa/netronome/inst_codegen.h`).

- The idea behind this code file is: to ease implementation and reduce the chance for bugs, use C preprocessor directives to programmatically generate the definitions for a `compute_*` function and a respective `predicate_*` function from the same base template. For some complicated operations, like determining whether the addition of two numbers causes an overflow, this isn't really possible, so I just defined the apropriate pair of functions manually. 
- The functions `compute_carry` and `predicate_carry` evaluate and express whether the ADDITION of two numbers results in a carry out. I don't have corresponding functions for subtraction because I'm not sure what the Netronome card's behavior on subtraction overflow is.

## Parsing a Netronome program

This isn't complete. The only important thing implemented in this direction so far are the vectors of strings in `src/isa/netronome/inst_parse.h`, which contain the string names for different parts of Netronome instructions.

## Program-level formalization

This is the functionality that takes in an entire netronome program and creates a formal expression for it, renaming register names where apropriate so the result is static single assignment (SSA). The test file is `src/verify/smt_prog_test_netronome.cc`. 

- I can't yet formalize programs with the `+carry` or `-carry` ALU operations, since I haven't built the machinery to do SSA transformations on the new carry-out state variable. To do this, you'd need to change some of the methods for the class `smt_var_base` and its child class `smt_var`, which are implemented in `src/isa/netronome/inst_var.h` and `inst_var.cc`.
- When conditional jumps are implemented, tests that check that the program's basic blocks are connected together the right way should go in this test file too.

## Validator

This is the functionality that verifies that two programs are equivalent, building on the program-level formalization functionality. It was pretty straightforward to get this working after program-level formalization worked. The test file is `src/verify/validator_test_netronome.cc`.

## Proposal generation for program synthesis

This tests the four different functions that take a Netronome program and make a random change to it for stochastic superoptimization. The test file is `src/search/proposals_test_netronome.cc`. The four functions are defined in `src/search/proposals.h`.

## Program synthesis

This actually generates a more optimal program from the original. The test is `src/search/mh_prog_test_netronome.cc`. The test compiles but doesn't actually generate meaningful programs--I think because there are parameters here that need to be tuned.

## Next steps in order of difficulty

- Implement the `-carry` ALU instruction
- Implement SSA for the carry out state variable so that we can formalize and verify instructions using `+carry` and `-carry`
- Implement the `alu_shf` operations (bit shifting)
	- The `<<indirect` and `>>indireect` op types depend on the A operand of the previous ALU operation
	- When this is done, the frameork should be able to do the calculations in the example program 1 at the back of the Netrnome Programmer's Reference Manual. This would be pretty cool :)
- Get program synthesis to actually work
- Implement condition flags and conditional jumps