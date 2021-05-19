
import argparse
import os
import subprocess

def extract_base_name(name):
    return name.split('.')[0] 

def parse_k2file(k2file):
    k2args = []
    with open(k2file) as f:
        for line in f:
            if len(line.strip()) == 0:
                # Skip blank lines
                continue
            line = line.replace(' ','')
            line = line.replace('\n','')
            line_arr = line.split('=')
            if len(line_arr[0]) == 1:
                line_arr[0] = f'-{line_arr[0]}'
            else:
                line_arr[0] = f'--{line_arr[0]}'
            k2args.append(line_arr[0])
            if len(line_arr) == 2:
                k2args.append(line_arr[1])
            elif len(line_arr) != 1:
                raise Exception('Error: Invalid K2 input format')
    k2args[-1] = k2args[-1][2:] # the last line is output path, remove "--"
    print(k2args)
    return k2args

def invoke_k2(descfile, name, k2args):

    insns_file = f'{name}.insns' 
    maps_file = f'{name}.maps' 
    k2_command = [ 
            './main_ebpf.out',
            '--bm_from_file',
            '--desc',
            descfile,
            '--bytecode',
            insns_file,
            '--map',
            maps_file,
    ]
    k2_command.extend(k2args[:-1])
    f_out = open(k2args[-1], "w")
    try:
        k2_output = subprocess.call(k2_command, stdout=f_out)
        # print(k2_output.decode('utf-8'))
    except subprocess.CalledProcessError as e:
        print(e.output)
        print('K2 failed')
        f_out.close()
        exit()
    f_out.close()

def invoke_text_extractor(infile):
    try:
        subprocess.check_output(['./elf_extract', infile ])
    except subprocess.CalledProcessError as e:
        print(e.output)
        print('Text extractor failed (is the input file correct?)')
        exit()
   
def invoke_patcher(infile, name):
    new_insns = 'output/output0.insns'
    try:
        subprocess.check_output([
            'python3', 
            'bpf-elf-tools/patch_insns/patch_elf.py', 
            infile,
            new_insns,
            name,
            '-o',
            f'output/{name}_modified.o'
        ]) 
    except subprocess.CalledProcessError as e:
        print(e.output)
        print('ELF patcher failed')
        exit()

def cleanup():
    try:
        with open(os.devnull) as FNULL:
            subprocess.check_output([
                'mkdir',
                'old-extracted-files',
            ], stderr=FNULL)
    except:
        pass
    for f in os.listdir('.'):
        if f.endswith('.insns') or f.endswith('.rel') or \
           f.endswith('.maps') or f.endswith('.txt'):
            try:
                subprocess.check_output([
                    'mv', 
                    f,
                    'old-extracted-files'
                ]) 
            except subprocess.CalledProcessError as e:
                print(e.output)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('infile')
    ap.add_argument('descfile')
    ap.add_argument('k2file')
    ap.add_argument('--programs', nargs='+', default=[])
    args = ap.parse_args()
    programs = args.programs
    infile = args.infile 
    descfile = args.descfile
    k2file = args.k2file
    invoke_text_extractor(infile)
    k2args = parse_k2file(k2file)
    if len(programs) > 0:
        for name in programs:
            invoke_k2(descfile, name, k2args)
            invoke_patcher(infile, name)
    else:
        for f in os.listdir('.'):
            if f.endswith('.insns'):
                name = extract_base_name(f)
                invoke_k2(descfile, name, k2args)
                invoke_patcher(infile, name)
    cleanup()

if __name__ == '__main__':
    main()

