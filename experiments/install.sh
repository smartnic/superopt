cd ../
echo "Compiling k2_inst translater..."
make k2_inst_translater.out
echo "Compiling K2..."
make main_ebpf.out
echo "Compiling z3 server..."
make z3server.out
cd experiments/

# Dependencies and installation for text extractor and patcher
sudo apt-get install libelf-dev
sudo apt-get install python3-pip

git clone https://github.com/smartnic/bpf-elf-tools.git
pip3 install -r bpf-elf-tools/patch_insns/requirements.txt

make -C bpf-elf-tools/text-extractor/ 
gcc  bpf-elf-tools/text-extractor/staticobjs/* -lelf -lz -o elf_extract

cp ../main_ebpf.out .
cp ../z3server.out .

mkdir -p src/isa/ebpf/
cp ../src/isa/ebpf/inst.runtime src/isa/ebpf/inst.runtime

