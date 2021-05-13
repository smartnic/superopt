
import argparse
import os
import subprocess

def extract_base_name(name):
    return name.split('.')[0] 

def invoke_k2(descfile, name):
    insns_file = f'{name}.insns' 
    maps_file = f'{name}.maps' 
    try:
        subprocess.check_output([
            './main_ebpf.out',
            '--bm_from_file',
            '--desc',
            descfile,
            '--bytecode',
            insns_file,
            '--map',
            maps_file,
            '--is_win',
            '--win_s_list',
            '5',
            '--win_e_list',
            '8',
            '-n',
            '10'
        ])
    except subprocess.CalledProcessError as e:
        print(e.output)
        print('K2 failed')
        exit()

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
    ap.add_argument('--programs', nargs='+', default=[])
    args = ap.parse_args()
    programs = args.programs
    infile = args.infile 
    descfile = args.descfile
    invoke_text_extractor(infile)
    if len(programs) > 0:
        for name in programs:
            invoke_k2(descfile, name)
            invoke_patcher(infile, name)
    else:
        for f in os.listdir('.'):
            if f.endswith('.insns'):
                name = extract_base_name(f)
                invoke_k2(descfile, name)
                invoke_patcher(infile, name)
    cleanup()

if __name__ == '__main__':
    main()

