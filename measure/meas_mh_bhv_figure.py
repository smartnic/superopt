#!/usr/bin/python3
import matplotlib.pyplot as plt
import networkx as nx
import getopt
import sys
import os
from collections import Counter

############### input output processing ########################

class InputParas:

    def __init__(self):
        self.fin_path = ""
        self.fout_path = ""
        self.niter = 0
        self.steady_start = 0
        self.bm_ids = []
        self.best_perf_costs = []
        self.w_e_list = []
        self.w_p_list = []
        self.st_list = []

    def print_out(self):
        print("fin_path:", self.fin_path)
        print("fout_path:", self.fout_path)
        print("niter:", self.niter)
        print("steady_start:", self.steady_start)
        print("bm_ids:", self.bm_ids)
        print("best_perf_costs:", self.best_perf_costs)
        print("w_e_list:", self.w_e_list)
        print("w_p_list:", self.w_p_list)
        print("st_list:", self.st_list)


def parse_input():
    in_para = InputParas()
    short_opts = "n:"
    long_opts = ["fin_path=", "bm_ids=", "w_list=", "st_list=",
                 "best_perf_costs=", "steady_start="]
    try:
        opts, args = getopt.getopt(sys.argv[1:], short_opts, long_opts)
    except getopt.GetoptError as err:
        print(str(err))
        sys.exit(2)
    for o, a in opts:
        if o == "-n":
            in_para.niter = a
        elif o == "--fin_path":
            in_para.fin_path = a
            in_para.fout_path = a + "../figure/"
        elif o == "--bm_ids":
            in_para.bm_ids = a.split(" ")
        elif o == "--w_list":
            w_list = a.split(" ")
            for w in w_list:
                w = w.split(",")
                in_para.w_e_list.append(w[0])
                in_para.w_p_list.append(w[1])
        elif o == "--st_list":
            in_para.st_list = a.split(" ")
        elif o == "--best_perf_costs":
            in_para.best_perf_costs = a.split(" ")
        elif o == "--steady_start":
            in_para.steady_start = a
        else:
            assert False, "unhandled option"
    return in_para


def create_fout_path(fout_path):
    folder = os.path.exists(fout_path)
    if not folder:
        os.makedirs(fout_path)


def get_all_data_from_file(file_name):
    with open(file_name, 'r') as f:
        lines = f.readlines()
        values = lines[0].strip('\n').split(' ')
        data = [[] for _ in range(len(values))]
        # the first line is format description
        for line in lines[1:]:
            line = line.strip('\n')
            values = line.split(' ')
            for i, val in enumerate(values):
                data[i].append(val)
    return data


class FileNameType:
    def __init__(self):
        self.proposals = "proposals"
        self.programs = "programs"
        self.optimals = "optimals"


file_name_type = FileNameType()


def get_input_file_name(fin_path, file_type, bm_id, niter, st, w_e, w_p):
    prefix = "raw_data"
    if file_type in [file_name_type.programs, file_name_type.proposals]:
        file_name = fin_path + prefix + "_" + file_type + "_" + bm_id + "_" + \
                    niter + "_" + st + "_" + w_e + "_" + w_p + ".txt"
    elif file_type == file_name_type.optimals:
        file_name = fin_path + prefix + "_" + file_type + "_" + bm_id + ".txt"
    return file_name


def get_output_figure_file_name(fout_path, list):
    figure_name = fout_path
    for x in list[:-1]:
        figure_name += x + "_"
    figure_name += list[-1] + ".pdf"
    return figure_name


# <accepted?> <error cost> <perf cost> <relative coding> <absolute coding>
class Proposals:

    def __init__(self, input_file_name):
        raw_data = get_all_data_from_file(input_file_name)
        self.accepted_list = raw_data[0]
        self.error_cost_list = raw_data[1]
        self.perf_cost_list = raw_data[2]
        self.rel_coding_list = raw_data[3]
        self.abs_coding_list = raw_data[4]

    def print_out(self):
        print(len(self.accepted_list), self.accepted_list)
        print(len(self.error_cost_list), self.error_cost_list)
        print(len(self.perf_cost_list), self.perf_cost_list)
        print(len(self.rel_coding_list), self.rel_coding_list)
        print(len(self.abs_coding_list), self.abs_coding_list)


# <iter num> <error cost> <perf cost> <relative coding> <absolute coding>
class Programs:

    def __init__(self, input_file_name):
        raw_data = get_all_data_from_file(input_file_name)
        self.iter_num_list = raw_data[0]
        self.error_cost_list = raw_data[1]
        self.perf_cost_list = raw_data[2]
        self.rel_coding_list = raw_data[3]
        self.abs_coding_list = raw_data[4]

    def print_out(self):
        print(len(self.iter_num_list), self.iter_num_list)
        print(len(self.error_cost_list), self.error_cost_list)
        print(len(self.perf_cost_list), self.perf_cost_list)
        print(len(self.rel_coding_list), self.rel_coding_list)
        print(len(self.abs_coding_list), self.abs_coding_list)


# <absolute coding>
class Optimals:

    def __init__(self, input_file_name):
        raw_data = get_all_data_from_file(input_file_name)
        self.abs_coding_list = raw_data[0]

    def print_out(self):
        print(len(self.abs_coding_list), self.abs_coding_list)


def get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p):
    fin_name = get_input_file_name(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    print("...processing " + fin_name)
    if file_type == file_name_type.programs:
        x = Programs(fin_name)
    elif file_type == file_name_type.proposals:
        x = Proposals(fin_name)
    elif file_type == file_name_type.optimals:
        x = Optimals(fin_name)
    return x


################ data processing ########################

class CostType:
    def __init__(self):
        self.error_cost = "error cost"
        self.perf_cost = "perf cost"
        self.total_cost = "total cost"


cost_type = CostType()

def get_total_cost_list(w_e, w_p, error_cost_list, perf_cost_list):
    total_cost_list = []
    for e, p in zip(error_cost_list, perf_cost_list):
        total_cost_list.append(float(w_e) * float(e) + float(w_p) * float(p))
    return total_cost_list


def cdf_xy(lst, max_cost_in_figure):
    val_number_dic = Counter(lst)
    val_list = sorted(val_number_dic.keys())
    max_x_index = len(val_list)
    for i, x in enumerate(val_list):
        if x >= max_cost_in_figure:
            max_x_index = i
            break
    value_list = val_list[:max_x_index]
    number_list = [val_number_dic[value_list[0]]]
    for i, x in enumerate(value_list[1:]):
        number_list.append(number_list[i] + val_number_dic[x])
    total_number = len(lst)
    cdf_list = [float(n) / float(total_number) for n in number_list]
    x_axis = [val_list[0]]
    y_axis = [cdf_list[0]]
    for x, y in zip(val_list, cdf_list):
        x_axis.append(x)
        y_axis.append(y_axis[-1])
        x_axis.append(x)
        y_axis.append(y)
    return x_axis, y_axis


def cost_function_cdf_xy(cost_list, max_cost_in_figure):
    cost_list = [float(cost) for cost in cost_list]
    x_axis, y_axis = cdf_xy(cost_list, max_cost_in_figure)
    return x_axis, y_axis


def error_cost_function_cdf_xy(error_cost_list):
    MAX_ERROR_COST_IN_FIGURE = 40
    return cost_function_cdf_xy(error_cost_list, MAX_ERROR_COST_IN_FIGURE)


def perf_cost_function_cdf_xy(perf_cost_list):
    MAX_PERF_COST_IN_FIGURE = 14
    return cost_function_cdf_xy(perf_cost_list, MAX_PERF_COST_IN_FIGURE)


def total_cost_function_cdf_xy(error_cost_list, perf_cost_list, w_e, w_p):
    MAX_TOTAL_COST_IN_FIGURE = 40
    total_cost_list = get_total_cost_list(w_e, w_p, error_cost_list, perf_cost_list)
    x_axis, y_axis = cost_function_cdf_xy(total_cost_list, MAX_TOTAL_COST_IN_FIGURE)
    return x_axis, y_axis


def cost_function_for_different_types_when_steady(c_type, file_type, fin_path, steady_start,
                                                  bm_id, niter, st, w_e, w_p):
    file_data = get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    steady_start_index = 0
    if file_type == file_name_type.programs:
        for i, iter in enumerate(file_data.iter_num_list):
            if int(iter) >= int(steady_start):
                steady_start_index = i
                break
    elif file_type == file_name_type.proposals:
        steady_start_index = int(steady_start)
    if c_type == cost_type.error_cost:
        x_axis, y_axis = error_cost_function_cdf_xy(file_data.error_cost_list[steady_start_index:])
    elif c_type == cost_type.perf_cost:
        x_axis, y_axis = perf_cost_function_cdf_xy(file_data.perf_cost_list[steady_start_index:])
    elif c_type == cost_type.total_cost:
        x_axis, y_axis = total_cost_function_cdf_xy(file_data.error_cost_list[steady_start_index:],
                                                    file_data.perf_cost_list[steady_start_index:],
                                                    w_e, w_p)
    return x_axis, y_axis


def best_perf_cost_with_zero_cost_for_programs(file_data):
    error_cost_list = [float(c) for c in file_data.error_cost_list]
    perf_cost_list = [float(c) for c in file_data.perf_cost_list]
    iter_num_list = [int(i) for i in file_data.iter_num_list]
    min_perf = perf_cost_list[0]
    x_axis = [0]
    y_axis = [min_perf]
    for iter_num, e, p in zip(iter_num_list, error_cost_list, perf_cost_list):
        if e == 0 and p < min_perf:
            y_axis.append(p)
            x_axis.append(iter_num)
            min_perf = p
    return x_axis, y_axis


def best_perf_cost_with_zero_cost_for_proposals(file_data):
    error_cost_list = [float(c) for c in file_data.error_cost_list]
    perf_cost_list = [float(c) for c in file_data.perf_cost_list]
    min_perf = perf_cost_list[0]
    x_axis = [0]
    y_axis = [min_perf]
    for (i, e), p in zip(enumerate(error_cost_list), perf_cost_list):
        if e == 0 and p < min_perf:
            y_axis.append(p)
            x_axis.append(i)
            min_perf = p
    return x_axis, y_axis


def best_perf_cost_with_zero_cost_for_different_types(file_type, fin_path, bm_id, niter, st, w_e, w_p):
    file_data = get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    if file_type == file_name_type.programs:
        iter_num_list = [int(i) for i in file_data.iter_num_list]
    elif file_type == file_name_type.proposals:
        iter_num_list = range(int(niter))
    MAX_PERF_COST = 14
    best_perf = MAX_PERF_COST
    for e, p in zip(file_data.error_cost_list, file_data.perf_cost_list):
        if float(e) == 0 and best_perf > float(p):
            best_perf = float(p)
    e = float(file_data.error_cost_list[0])
    p = float(file_data.perf_cost_list[0])
    if e == 0:
        min_perf = p
    else:
        min_perf = MAX_PERF_COST
    y_axis = []  # best_perf_cost
    x_axis = []  # iteration number
    index = 1
    next_iter = int(iter_num_list[index])
    for i in range(int(niter)):
        x_axis.append(i)
        if next_iter == i:
            e = float(file_data.error_cost_list[index])
            p = float(file_data.perf_cost_list[index])
            index += 1
            if index >= len(iter_num_list):
                next_iter = len(x_axis)  # set as num_iter
            else:
                next_iter = int(iter_num_list[index])
            if e == 0 and p < min_perf:
                min_perf = p
            y_axis.append(min_perf)
            if min_perf == best_perf:
                break
        else:
            y_axis.append(min_perf)
    return x_axis, y_axis


def num_unique_programs_over_iterations(iter_num_list, abs_coding_list):
    iter_num_list = [int(x) for x in iter_num_list]
    abs_coding_list = [int(x, 2) for x in abs_coding_list]
    abs_coding_set = {abs_coding_list[0]}
    x_axis = [0]
    y_axis = [1]
    for iter_num, abs_coding in zip(iter_num_list[1:], abs_coding_list[1:]):
        if abs_coding not in abs_coding_set:
            x_axis.append(iter_num)
            y_axis.append(y_axis[-1] + 1)
            abs_coding_set.add(abs_coding)
    return x_axis, y_axis


def num_unique_programs_for_different_types(file_type, fin_path, bm_id, niter, st, w_e, w_p):
    file_data = get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    if file_type == file_name_type.programs:
        x_axis, y_axis = num_unique_programs_over_iterations(file_data.iter_num_list, file_data.abs_coding_list)
    elif file_type == file_name_type.proposals:
        iter_num_list = list(range(len(file_data.abs_coding_list)))
        x_axis, y_axis = num_unique_programs_over_iterations(iter_num_list, file_data.abs_coding_list)
    return x_axis, y_axis


def get_abs_opcodes_operands_for_different_types(file_type, fin_path, bm_id, niter, st, w_e, w_p):
    file_data = get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    abs_bv_list = file_data.abs_coding_list
    opcode_list = []
    operand_list = []
    OP_LEN = 5
    for prog_bv in abs_bv_list:
        inst_bv_list = [prog_bv[i: i + OP_LEN * 4] for i in range(0, len(prog_bv), OP_LEN * 4)]
        prog_opcode_bv = ""
        prog_operand_bv = ""
        for inst_bv in inst_bv_list:
            prog_opcode_bv += inst_bv[:OP_LEN]
            prog_operand_bv += inst_bv[OP_LEN:]
        opcode_list.append(prog_opcode_bv)
        operand_list.append(prog_operand_bv)
    opcode_list = [int(i, 2) for i in opcode_list]
    operand_list = [int(i, 2) for i in operand_list]
    return opcode_list, operand_list


#################### figure plotting ####################
def figure_cost_function_cdf_when_steady(file_type, c_type, fin_path, fout_path, niter,
                                         steady_start, bm_id, best_perf_cost, st, w_e_list, w_p_list):
    f = plt.figure()
    for w_e, w_p in zip(w_e_list, w_p_list):
        x_axis, y_axis = cost_function_for_different_types_when_steady(c_type, file_type,
                                                                       fin_path, steady_start,
                                                                       bm_id, niter, st, w_e, w_p)
        curve_name = " w_e:" + w_e + " w_p:" + w_p
        plt.plot(x_axis, y_axis, linestyle='-.', linewidth=1.5, label=curve_name, marker='x')
    graph_title = file_type + " " + c_type + " function CDF"
    graph_title_suffix = "\nbm:" + bm_id + " niter:" + niter + " strategy:" + \
                         st + " best perf cost:" + best_perf_cost
    plt.title(graph_title + graph_title_suffix)
    plt.xlabel(c_type)
    plt.ylabel('CDF')
    plt.legend()
    plt.grid()
    figure_name = get_output_figure_file_name(fout_path, [graph_title, bm_id, niter, st])
    f.savefig(figure_name, bbox_inches='tight')
    print("figure output : " + figure_name)
    plt.close(f)


def figure_best_perf_cost_with_zero_error_cost(file_type, fin_path, fout_path, niter,
                                               bm_id, best_perf_cost, st, w_e_list, w_p_list):
    f = plt.figure()
    for w_e, w_p in zip(w_e_list, w_p_list):
        x_axis, y_axis = best_perf_cost_with_zero_cost_for_different_types(file_type, fin_path, bm_id,
                                                                           niter, st, w_e, w_p)
        curve_name = " w_e:" + w_e + " w_p:" + w_p
        plt.plot(x_axis, y_axis, linestyle='-.', linewidth=1.5, label=curve_name, marker='x')
    graph_title = file_type + " best perf cost with zero error cost over iterations"
    graph_title_suffix = "\nbm:" + bm_id + " niter:" + niter + " strategy:" + \
                         st + " best perf cost:" + best_perf_cost
    plt.title(graph_title + graph_title_suffix)
    plt.xlabel('Iteration number')
    plt.ylabel('Perf cost with zero error cost')
    plt.legend()
    plt.grid()
    figure_name = get_output_figure_file_name(fout_path, [graph_title, bm_id, niter, st])
    f.savefig(figure_name, bbox_inches='tight')
    print("figure output : " + figure_name)
    plt.close(f)


def figure_num_unique_programs(file_type, fin_path, fout_path, niter, bm_id, st, w_e_list, w_p_list):
    f = plt.figure()
    for w_e, w_p in zip(w_e_list, w_p_list):
        x_axis, y_axis = num_unique_programs_for_different_types(file_type, fin_path, bm_id, niter, st, w_e, w_p)
        curve_name = " w_e:" + w_e + " w_p:" + w_p + " total #unique_programs:" + str(y_axis[-1])
        plt.plot(x_axis, y_axis, linestyle='-', linewidth=1.5, label=curve_name)
    graph_title = file_type + " number of unique programs over iterations"
    graph_title_suffix = "\nbm:" + bm_id + " niter:" + niter + " strategy:" + st
    plt.title(graph_title + graph_title_suffix)
    plt.xlabel('Iteration number')
    plt.ylabel('Number of unique programs')
    plt.legend()
    plt.grid()
    figure_name = get_output_figure_file_name(fout_path, [graph_title, bm_id, niter, st])
    f.savefig(figure_name, bbox_inches='tight')
    print("figure output : " + figure_name)
    plt.close(f)


def figure_absolute_coding_transfer_graph(file_type, fin_path, fout_path, niter, bm_id, st, w_e, w_p):
    opcode_list, operand_list = get_abs_opcodes_operands_for_different_types(file_type, fin_path, bm_id,
                                                                             niter, st, w_e, w_p)
    figsize_x = 16
    figsize_y = 8
    f = plt.figure(figsize=(figsize_x, figsize_y))
    plt.subplot(1, 2, 1)
    plt.scatter(operand_list, opcode_list, marker='o',
                c="blue", alpha=1)
    # compute the optimal program points and add them into the figure
    opti_opcode_list, opti_operand_list = get_abs_opcodes_operands_for_different_types(file_name_type.optimals,
                                                                                       fin_path, bm_id,
                                                                                       niter, st, w_e, w_p)
    plt.scatter(opti_operand_list, opti_opcode_list, marker='*', c="tab:red", alpha=0.1)
    graph_title = file_type + " absolute coding transfer graph"
    graph_title_suffix = "\nbm:" + bm_id + " niter:" + niter + " strategy:" + \
                         st + "\nbest perf cost:" + best_perf_cost + " w_e:" + w_e + " w_p:" + w_p
    plt.title(graph_title + graph_title_suffix)
    plt.xlabel('Operand bit vector')
    plt.ylabel('Opcode bit vector')
    plt.yscale('log')
    plt.xscale('log')
    plt.ylim(ymin=1)
    plt.xlim(xmin=1)
    plt.grid()
    plt.subplot(1, 2, 2)
    plt.scatter(operand_list, opcode_list, marker='o', c="blue", alpha=1)
    plt.scatter(opti_operand_list, opti_opcode_list, marker='*', c="tab:red", alpha=0.1)
    plt.title(graph_title + graph_title_suffix)
    plt.xlabel('Operand bit vector')
    plt.ylabel('Opcode bit vector')
    plt.yscale('log')
    plt.xscale('log')
    plt.ylim(ymin=pow(10, 7))
    plt.grid()
    figure_name = get_output_figure_file_name(fout_path, [graph_title, bm_id, niter, st, w_e, w_p])
    f.savefig(figure_name, bbox_inches='tight')
    print("figure output : " + figure_name)
    plt.close(f)

########################## relative coding transfer graph #####################################

class NodeInfo:

    def __init__(self):
        self.freq_count = 0
        self.num_unique_program_count = 0
        self.x_pos = None
        self.y_pos = None
        self.avg_legal_error_cost = None
        self.avg_legal_perf_cost = None
        self.avg_legal_total_cost = None


def rm_empty_node_list_in(nodes):
    new_nodes = []
    for i, ns in enumerate(nodes):
        if len(ns) > 0:
            new_nodes.append(ns)
    return new_nodes


def get_freq_count_list(file_data, file_type, niter):
    if file_type == file_name_type.proposals:
        freq_count_list = list([1] * len(file_data.rel_coding_list))
    elif file_type == file_name_type.programs:
        freq_count_list = []
        for i, v in enumerate(file_data.iter_num_list[:-1]):
            freq_count_list.append(int(file_data.iter_num_list[i + 1]) - int(v))
        if file_data.iter_num_list[-1] == niter:
            freq_count_list.append(1)
        else:
            freq_count_list.append(int(niter) - int(file_data.iter_num_list[-1]))
    return freq_count_list


def add_freq_count_in_nodes_info(file_data, file_type, niter, nodes_info):
    freq_count_list = get_freq_count_list(file_data, file_type, niter)
    for a, b in zip(file_data.rel_coding_list, freq_count_list):
        nodes_info[a].freq_count += b
    return nodes_info


def add_node_pos_in_nodes_info(nodes_info, nodes):
    # compute x pos for nodes and store it into node_info
    x_max = float(max([len(x) for x in nodes]))
    for ns in nodes:
        for i, n in enumerate(ns):
            gap = x_max / (len(ns) + 1)
            nodes_info[n].x_pos = float(i) * gap + gap
    # compute y pos for nodes and store it into node_info
    for k in nodes_info.keys():
        nodes_info[k].y_pos = k.count('0')
    return nodes_info


def get_nodes_cost_dic_for_nodes_info(cost, max_cost, file_data, file_type, niter):
    freq_count_list = get_freq_count_list(file_data, file_type, niter)
    # key: node_name(rel_coding); total cost for this node, frequency count of this node
    node_cost_dic = {}
    for c, rel_coding, freq_count in zip(cost, file_data.rel_coding_list, freq_count_list):
        if c >= max_cost:
            continue
        if rel_coding in node_cost_dic:
            node_cost_dic[rel_coding]["total_cost"] += c * freq_count
            node_cost_dic[rel_coding]["freq_count"] += freq_count
        else:
            node_cost_dic[rel_coding] = {"total_cost": c * freq_count,
                                         "freq_count": freq_count}
    return node_cost_dic


def add_nodes_error_cost_in_nodes_info(max_cost, file_data, file_type, niter, nodes_info):
    err_cost_list = [float(e) for e in file_data.error_cost_list]
    node_error_cost_dic = get_nodes_cost_dic_for_nodes_info(err_cost_list, max_cost, file_data, file_type, niter)
    for k in nodes_info.keys():
        if k in node_error_cost_dic:
            nodes_info[k].avg_legal_error_cost = round(node_error_cost_dic[k]["total_cost"]
                                                       / node_error_cost_dic[k]["freq_count"])
        else:
            nodes_info[k].avg_legal_error_cost = max_cost
    return nodes_info


def add_nodes_perf_cost_in_nodes_info(max_cost, file_data, file_type, niter, nodes_info):
    perf_cost_list = [float(e) for e in file_data.perf_cost_list]
    node_perf_cost_dic = get_nodes_cost_dic_for_nodes_info(perf_cost_list, max_cost, file_data, file_type, niter)
    for k in nodes_info.keys():
        if k in node_perf_cost_dic:
            nodes_info[k].avg_legal_perf_cost = round(node_perf_cost_dic[k]["total_cost"]
                                                      / node_perf_cost_dic[k]["freq_count"])
        else:
            nodes_info[k].avg_legal_perf_cost = max_cost
    return nodes_info


def add_nodes_total_cost_in_nodes_info(max_cost, file_data, file_type, niter, nodes_info, w_e, w_p):
    total_cost_list = get_total_cost_list(w_e, w_p, file_data.error_cost_list, file_data.perf_cost_list)
    node_total_cost_dic = get_nodes_cost_dic_for_nodes_info(total_cost_list, max_cost, file_data, file_type, niter)
    for k in nodes_info.keys():
        if k in node_total_cost_dic:
            nodes_info[k].avg_legal_total_cost = round(node_total_cost_dic[k]["total_cost"]
                                                       / node_total_cost_dic[k]["freq_count"])
        else:
            nodes_info[k].avg_legal_total_cost = max_cost
    return nodes_info


def add_unique_program_count_in_nodes_info(nodes_info, file_data):
    rel_coding_unique_count_dic = {}
    abs_coding_set = set()
    rel_coding_list = file_data.rel_coding_list
    abs_coding_list = file_data.abs_coding_list
    for a, b in zip(rel_coding_list, abs_coding_list):
        if b not in abs_coding_set:
            abs_coding_set.add(b)
            if a not in rel_coding_unique_count_dic:
                rel_coding_unique_count_dic[a] = 1
            else:
                rel_coding_unique_count_dic[a] += 1
    for k in nodes_info.keys():
        nodes_info[k].num_unique_program_count = rel_coding_unique_count_dic[k]
    return nodes_info


def gen_nodes_info_for_rel_coding_transfer_graph(nodes, file_data, file_type, max_err_cost, niter, w_e, w_p):
    nodes_info = {}
    for ns in nodes:
        for n in ns:
            nodes_info[n] = NodeInfo()
    nodes_info = add_freq_count_in_nodes_info(file_data, file_type, niter, nodes_info)
    # compute (x,y) pos for nodes and store it into node_info
    nodes_info = add_node_pos_in_nodes_info(nodes_info, nodes)
    # compute avg error cost (VALID PROGRAM, ROUNDED) and store it into node_info
    nodes_info = add_nodes_error_cost_in_nodes_info(max_err_cost, file_data, file_type, niter, nodes_info)
    # compute avg perf cost (VALID PROGRAM, ROUNDED) and store it into node_info
    nodes_info = add_nodes_perf_cost_in_nodes_info(max_err_cost, file_data, file_type, niter, nodes_info)
    # compute avg total cost (VALID PROGRAM, ROUNDED) and store it into node_info
    nodes_info = add_nodes_total_cost_in_nodes_info(max_err_cost, file_data, file_type, niter, nodes_info, w_e, w_p)
    # compute #unique program and store it into node_info
    nodes_info = add_unique_program_count_in_nodes_info(nodes_info, file_data)
    return nodes_info


# according to the fmt generated by function `gen_nodes_info`
def get_node_name_in_rel_transfer_graph(node_name, info):
    name = node_name + "\n" + \
           str(info.freq_count) + " " + str(info.num_unique_program_count) + "\n" + \
           str(info.avg_legal_total_cost) + " " + str(info.avg_legal_error_cost) + " " + str(info.avg_legal_perf_cost)
    return name


def get_node_pos(info):
    pos = (info.x_pos, info.y_pos)
    return pos


def gen_node_color_for_rel_coding_transfer_graph(nodes_info):
    node_color = []
    for _, v in nodes_info.items():
        node_color.append(v.freq_count)
    return node_color


def gen_edges_for_rel_coding_transfer_graph(nodes_info, nodes_list):
    edges_set = set()
    n1 = nodes_list[0]
    for n in nodes_list[1:]:
        if n1 != n:
            e = (get_node_name_in_rel_transfer_graph(n1, nodes_info[n1]),
                 get_node_name_in_rel_transfer_graph(n, nodes_info[n]))
            edges_set.add(e)
            n1 = n
    edges = list(edges_set)
    return edges


def add_avg_legal_cost_for_diff_layers_in_text_nodes(text_nodes, max_cost, file_type, file_data, nodes, niter, w_e, w_p):
    error_cost_list = [float(c) for c in file_data.error_cost_list]
    error_cost_dic = get_nodes_cost_dic_for_nodes_info(error_cost_list, max_cost, file_data, file_type, niter)
    perf_cost_list = [float(c) for c in file_data.perf_cost_list]
    perf_cost_dic = get_nodes_cost_dic_for_nodes_info(perf_cost_list, max_cost, file_data, file_type, niter)
    for i, ns in enumerate(nodes):
        err_value = 0
        perf_value = 0
        count = 0
        for n in ns:
            if n in error_cost_dic:
                err_value += error_cost_dic[n]["total_cost"]
                perf_value += perf_cost_dic[n]["total_cost"]
                count += error_cost_dic[n]["freq_count"]
            else:
                print("only illegal value in this node")
        if count == 0:
            print("only illegal value in layer ", i)
            text_nodes[i].avg_legal_error_cost = max_cost
            text_nodes[i].avg_legal_perf_cost = max_cost
            text_nodes[i].avg_legal_total_cost = max_cost
        else:
            text_nodes[i].avg_legal_error_cost = round(err_value / count)
            text_nodes[i].avg_legal_perf_cost = round(perf_value / count)
            text_nodes[i].avg_legal_total_cost = round((float(w_e) * err_value + float(w_p) * perf_value) / count)
    return text_nodes


def gen_text_node_in_rel_transfer_graph(nodes, nodes_info, max_cost, file_type, file_data, niter, w_e, w_p):
    text_nodes = [NodeInfo() for _ in range(len(nodes))]
    max_x_pos = max([x.x_pos for x in nodes_info.values()])
    min_x_pos = min([x.x_pos for x in nodes_info.values()])
    x_pos_gap = (max_x_pos - min_x_pos) / len(nodes_info)
    NUM_X_POS_GAP = 8
    x_pos = min_x_pos - NUM_X_POS_GAP * x_pos_gap
    for i, ns in enumerate(nodes):
        count = 0
        for n in ns:
            count += nodes_info[n].freq_count
        text_nodes[i].x_pos = x_pos
        text_nodes[i].y_pos = ns[0].count('0')
        text_nodes[i].freq_count = count

    text_nodes = add_avg_legal_cost_for_diff_layers_in_text_nodes(text_nodes, max_cost, file_type,
                                                                  file_data, nodes, niter, w_e, w_p)
    return text_nodes


def figure_relative_coding_transfer_graph(file_type, fin_path, fout_path, niter, bm_id, st, w_e, w_p):
    file_data = get_data_for_different_file_types(fin_path, file_type, bm_id, niter, st, w_e, w_p)
    nodes_list = file_data.rel_coding_list
    nodes_list_ordered = sorted(list(set(nodes_list)))
    # `nodes`: each item in `nodes` is a node list where each node has
    # the same number of `1` in its relative coding as others
    nodes = [[] for _ in range(len(nodes_list_ordered[0]) + 1)]
    for n in nodes_list_ordered:
        nodes[n.count('1')].append(n)
    nodes = rm_empty_node_list_in(nodes)
    # compute nodes_info
    MAX_ERR_COST = 100000
    nodes_info = gen_nodes_info_for_rel_coding_transfer_graph(nodes, file_data, file_type,
                                                              MAX_ERR_COST, niter, w_e, w_p)
    node_color = gen_node_color_for_rel_coding_transfer_graph(nodes_info)
    edges = gen_edges_for_rel_coding_transfer_graph(nodes_info, nodes_list)
    # draw graph
    figsize_x = max(0.35 * max(len(elem) for elem in nodes), 4)
    figsize_y = figsize_x * 0.75
    f = plt.figure(figsize=(figsize_x, figsize_y))
    g = nx.DiGraph()
    # add node into graph
    nodes_name = []
    for k, v in nodes_info.items():
        nodes_name.append(get_node_name_in_rel_transfer_graph(k, v))
        g.add_node(get_node_name_in_rel_transfer_graph(k, v), pos=get_node_pos(v))
    # add edge into graph
    g.add_edges_from(edges)
    pos = nx.get_node_attributes(g, 'pos')
    nx.draw(g, pos=pos, with_labels=True, node_size=350,
            font_size=3.8, node_color=node_color, cmap=plt.cm.Reds,
            edge_color='black', width=0.2, arrowstyle='->')
    text_nodes = gen_text_node_in_rel_transfer_graph(nodes, nodes_info, MAX_ERR_COST, file_type,
                                                     file_data, niter, w_e, w_p)
    for tn in text_nodes:
        text = str(tn.freq_count) + "\n" + \
               str(tn.avg_legal_total_cost) + "\n" + \
               str(tn.avg_legal_error_cost) + " " + str(tn.avg_legal_perf_cost)
        plt.text(tn.x_pos, tn.y_pos, text, fontsize='x-small')
    graph_title = file_type + " relative coding transfer graph"
    graph_title_suffix = "\nbm:" + bm_id + " niter:" + niter + " strategy:" + \
                         st + "\nbest perf cost:" + best_perf_cost + " w_e:" + w_e + " w_p:" + w_p
    plt.title(graph_title + graph_title_suffix)
    figure_name = get_output_figure_file_name(fout_path, [graph_title, bm_id, niter, st, w_e, w_p])
    f.savefig(figure_name, bbox_inches='tight')
    print("figure output : " + figure_name)
    plt.close(f)


if __name__ == "__main__":
    in_para = parse_input()
    create_fout_path(in_para.fout_path)

    for bm_id, best_perf_cost in zip(in_para.bm_ids, in_para.best_perf_costs):
        for st in in_para.st_list:
            for file_type in [file_name_type.proposals, file_name_type.programs]:
                # figure 1: cost function CDF
                for c_type in [cost_type.error_cost, cost_type.perf_cost, cost_type.total_cost]:
                    figure_cost_function_cdf_when_steady(file_type, c_type, in_para.fin_path, in_para.fout_path,
                                                         in_para.niter, in_para.steady_start, bm_id, best_perf_cost,
                                                         st, in_para.w_e_list, in_para.w_p_list)
                # figure 2: best performance cost with zero error cost over iterations
                figure_best_perf_cost_with_zero_error_cost(file_type, in_para.fin_path, in_para.fout_path,
                                                           in_para.niter, bm_id, best_perf_cost, st,
                                                           in_para.w_e_list, in_para.w_p_list)
                # figure 3: the number of unique programs over iterations
                figure_num_unique_programs(file_type, in_para.fin_path, in_para.fout_path, in_para.niter,
                                           bm_id, st, in_para.w_e_list, in_para.w_p_list)
                for w_e, w_p in zip(in_para.w_e_list, in_para.w_p_list):
                    # figure 4: relative coding transfer graph
                    figure_relative_coding_transfer_graph(file_type, in_para.fin_path, in_para.fout_path,
                                                          in_para.niter, bm_id, st, w_e, w_p)
                    # figure 5: absolute coding transfer graph
                    figure_absolute_coding_transfer_graph(file_type, in_para.fin_path, in_para.fout_path,
                                                          in_para.niter, bm_id, st, w_e, w_p)
