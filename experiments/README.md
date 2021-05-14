
# K2 experiments

## Running K2 from object file

Install K2 and dependencies:

    ./install.sh

Now execute run_k2.py:

    python3 run_k2.py <object file> <desc file> <K2 file>

Example:

    python3 run_k2.py xdp1_kern.o xdp1.desc 


This will be default compile all BPF sections in the binary. To select specific section(s), run:

    python3 run_k2.py xdp1_kern.o xdp1.desc k2_args.txt --sections xdp1

For an example of the K2 args file, see k2-arg-files/xdp1-args.txt

