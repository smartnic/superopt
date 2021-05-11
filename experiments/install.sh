cd ../
echo "Compiling k2_inst translater..."
make k2_inst_translater.out
echo "Compiling k2..."
make main_ebpf.out
echo "Compiling z3 server..."
make z3server.out
cd experiments/
