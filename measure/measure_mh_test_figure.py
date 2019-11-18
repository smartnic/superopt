#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm
import sys
import networkx as nx
import matplotlib as mpl


def get_all_data_from_file(file_name):
    data = []
    num = 0
    with open(file_name, 'r') as f:
        lines = f.readlines()
        num = len(lines)
        line = lines[0].strip('\n')
        values = line.split(' ')
        # print(values)
        for _ in values:
            data.append([])
        # print(len(data))
        for line in lines:
            line = line.strip('\n')
            values = line.split(' ')
            for i, val in enumerate(values):
                data[i].append(val)
    return data, num


def get_data_from_file(file_name, i):
    data = []
    with open(file_name, 'r') as f:
        lines = f.readlines()
        line = lines[i]
        values = line.split(' ')
        for val in values:
            data.append(val)
        data.pop()
    # return data[:12000]
    return data


def separate_pair(pairs):
    val0 = []
    val1 = []
    for pair in pairs:
        val = pair.split(',')
        val0.append(val[0])
        val1.append(val[1])
    return val0, val1


def top_iter_xy(file_name, i):
    data = get_data_from_file(file_name, i)
    # print(data)
    data = np.array(data).astype(int)
    x_axis = []
    y_axis = []
    for num in data:
        x_axis.append(i)
        i += 1
        y_axis.append(num)
    # print(x_axis, y_axis)
    return x_axis, y_axis


def parse_input(input):
    fin_path = input[0]
    fout_path = input[1]
    parameters = input[2].split(',')
    parameters = [[float(parameters[i]), float(parameters[i + 1])] for i in range(0, len(parameters), 2)]
    prog_ids = input[3].split(',')
    best_perf_costs = input[4].split(',')
    best_perf_costs = [int(c) for c in best_perf_costs]
    steady_start = int(input[5])
    return fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start


def cost_function_xy(file_name, r):
    data, _ = get_all_data_from_file(file_name)
    cost_list = np.array(data[r]).astype(float)
    cost_list = np.sort(cost_list)
    cost_list = cost_list.tolist()
    # print(cost_list)
    i = 0
    x_axis = [cost_list[0]]
    y_axis = [1]
    for c in cost_list[1:]:
        # if c > 40:
        #     # x_axis.append(30)
        #     # y_axis.append(y_axis[i])
        #     break
        if c == x_axis[i]:
            y_axis[i] += 1
        else:
            x_axis.append(c)
            y_axis.append(y_axis[i] + 1)
            i += 1
    # print("x_axis:", x_axis)
    # print("y_axis:", y_axis)
    number = len(cost_list)
    # print(len(x_axis), x_axis)
    # print(len(y_axis), y_axis)
    for i, y in enumerate(y_axis):
        y_axis[i] = float(y) / number
    return x_axis, y_axis


def draw_error_cost_funtion(fin_path, fout_path, parameters, prog_ids):
    # error cost function
    for id in prog_ids:
        f = plt.figure()
        for para in parameters:
            file_name = fin_path + "raw_data_proposal_" + str(id) + "_" + \
                        str(para[0]).rstrip('0').rstrip('.') + "_" + \
                        str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_name)

            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1])
            x_axis, y_axis = cost_function_xy(file_name, 0)
            i = 0
            for j, x in enumerate(x_axis):
                i = j
                if x > 40:
                    break
            plt.plot(x_axis[:i], y_axis[:i], linestyle='-', linewidth=1.5, label=curve_name, marker='x')
        graph_title = "Error cost function distribution"
        graph_title_suffix = "\nprogram id=" + str(id) + " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        plt.xlabel('Error cost value')
        plt.ylabel('CDF')
        # plt.show()
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.legend()
        plt.grid()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        plt.close(f)


def draw_perf_cost_funtion(fin_path, fout_path, parameters, prog_ids):
    # cost function
    for i_id, id in enumerate(prog_ids):
        f = plt.figure()
        for para in parameters:
            file_name = fin_path + "raw_data_proposal_" + str(id) + "_" + \
                        str(para[0]).rstrip('0').rstrip('.') + "_" + \
                        str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_name)
            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1])
            fig_x_axis, fig_y_axis = cost_function_xy(file_name, 1)
            plt.plot(fig_x_axis, fig_y_axis, linestyle='-', linewidth=1.5, label=curve_name, marker='x')
        graph_title = "Performance cost function distribution"
        graph_title_suffix = "\nprogram id=" + str(id) + " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        plt.xlabel('Performance cost value')
        plt.ylabel('CDF')
        # plt.show()
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.legend()
        plt.grid()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        print("draw_perf_cost_funtion::fout:", fout_path + figurename)
        plt.close(f)


def draw_cost_funtion(fin_path, fout_path, parameters, prog_ids, best_perf_costs):
    # cost function
    for i_id, id in enumerate(prog_ids):
        f = plt.figure()
        for para in parameters:
            file_name = fin_path + "raw_data_proposal_" + str(id) + "_" + \
                        str(para[0]).rstrip('0').rstrip('.') + "_" + \
                        str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_name)
            fig_x_axis, fig_y_axis = cost_function_xy(file_name, 2)
            best_perf_cost = para[1] * float(best_perf_costs[int(id)])
            # print("best_perf_cost:", best_perf_cost)
            fig_x_axis = [x / best_perf_cost for x in fig_x_axis]
            i = 0
            for j, x in enumerate(fig_x_axis):
                i = j
                if x > 15:
                    break
            fig_x_axis = fig_x_axis[:i]
            fig_y_axis = fig_y_axis[:i]
            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1]) + \
                         " best cost value=" + str(best_perf_cost)
            plt.plot(fig_x_axis, fig_y_axis, linestyle='-', linewidth=1.5, label=curve_name, marker='x')
        graph_title = "Cost function distribution"
        graph_title_suffix = "\nprogram id=" + str(id) + " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        plt.xlabel('Cost value / Best cost value')
        plt.ylabel('CDF')
        # plt.show()
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.legend()
        plt.grid()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        print("draw_cost_funtion::fout:", fout_path + figurename)
        plt.close(f)


def draw_smallest_perf_cost(fin_path, fout_path, parameters, prog_ids, best_perf_costs):
    # smallest performance cost and iteration number for one program
    # prog_id is fixed, number of figures: len(prog_ids)
    for best_c, id in zip(best_perf_costs, prog_ids):
        f = plt.figure()
        for para in parameters:
            file_in = fin_path + "raw_data_prog_" + str(id) + "_" + \
                      str(para[0]).rstrip('0').rstrip('.') + "_" + \
                      str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_in)
            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1])
            raw_data, num_iter = get_all_data_from_file(file_in)
            error_cost = np.array(raw_data[0]).astype(float)
            perf_cost = np.array(raw_data[1]).astype(float)
            # print("perf_cost:", len(perf_cost), perf_cost)
            # iter_num
            x_axis = list(range(1, num_iter + 1, 1))
            # smalles_perf_cost
            y_axis = []
            if error_cost[0] == 0:
                y_axis.append(perf_cost[0])
            else:
                y_axis.append(14)  # max_perf_cost
            n = 10
            for i, c in enumerate(perf_cost):
                if i != 0:
                    if error_cost[i] == 0:
                        if c < y_axis[i - 1]:
                            n = i + 500
                        y_axis.append(min(c, y_axis[i - 1]))
                    else:
                        y_axis.append(y_axis[i - 1])
            plt.plot(x_axis[:n], y_axis[:n], linestyle='-.', linewidth=1.5, label=curve_name)
        graph_title = "Relationship between smallest performance cost and iteration number"
        graph_title_suffix = "\nprogram id=" + str(id) + " error cost=0\n optimal performance cost=" + str(best_c) + \
                             " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.xlabel('Iteration number')
        plt.ylabel('Smallest performance cost')
        plt.legend()
        plt.grid()
        # plt.show()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        print("draw_smallest_perf_cost::fout:", fout_path + figurename)
        plt.close(f)


def cost_sample_xy(file_name, i, steady_start):
    data, _ = get_all_data_from_file(file_name)
    cost_list = np.array(data[i]).astype(float)
    cost_list = cost_list[steady_start:]
    # print("cost_list:", cost_list)
    cost_list = np.sort(cost_list)
    cost_list = cost_list.tolist()
    i = 0
    x_axis = [cost_list[0]]
    y_axis = [1]
    for c in cost_list[1:]:
        if c == x_axis[i]:
            y_axis[i] += 1
        else:
            x_axis.append(c)
            y_axis.append(y_axis[i] + 1)
            i += 1
    number = len(cost_list)
    # print(len(x_axis), x_axis)
    # print(len(y_axis), y_axis)
    for i, y in enumerate(y_axis):
        y_axis[i] = float(y) / number
    return x_axis, y_axis


def draw_cost_value_of_generated_progs(fin_path, fout_path, parameters,
                                       prog_ids, best_perf_costs, steady_start):
    # cost value and number of samples figure for one program
    # prog_id is fixed
    for i_id, id in enumerate(prog_ids):
        f = plt.figure()
        for para in parameters:
            file_name = fin_path + "raw_data_prog_" + str(id) + "_" + \
                        str(para[0]).rstrip('0').rstrip('.') + "_" + \
                        str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_name)
            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1]) + \
                         ", best cost value=" + str(float(best_perf_costs[i_id]) * float(para[1]))
            x_axis, y_axis = cost_sample_xy(file_name, 2, steady_start)
            for i, x in enumerate(x_axis):
                x_axis[i] = x / (float(best_perf_costs[i_id]) * float(para[1]))
            # print(x_axis)
            plt.plot(x_axis, y_axis, linestyle='-.', linewidth=1.5, label=curve_name, marker='x')
        graph_title = "Total cost value CDF"
        graph_title_suffix = "\nprogram id=" + str(id) + " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        plt.xlabel('Cost value / Best cost value')
        plt.ylabel('Number of samples CDF')
        plt.ylim(ymin=0)
        plt.xlim(xmin=1)
        # plt.show()
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.legend()
        plt.grid()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        print("draw_cost_value_of_generated_progs::fout:", fout_path + figurename)
        plt.close(f)


# draw number of unique programs over iterations
def draw_num_unique_progs(fin_path, fout_path, parameters, prog_ids):
    for i_id, id in enumerate(prog_ids):
        f = plt.figure()
        for para in parameters:
            file_name = fin_path + "raw_data_prog_" + str(id) + "_" + \
                        str(para[0]).rstrip('0').rstrip('.') + "_" + \
                        str(para[1]).rstrip('0').rstrip('.') + ".txt"
            print("  processing", file_name)
            data, _ = get_all_data_from_file(file_name)
            y_axis = []
            sum = 0
            for freq in np.array(data[3]).astype(int):
                if freq == 1:
                    sum += 1
                y_axis.append(sum)
            x_axis = [x for x in range(len(y_axis))]
            for i, y in enumerate(y_axis):
                y_axis[i] = y / sum
            curve_name = "w_e=" + str(para[0]) + " w_p=" + str(para[1]) + \
                         " #total unique programs=" + str(sum)
            plt.plot(x_axis, y_axis, linestyle='-', linewidth=1.5, label=curve_name)
        graph_title = "Number of unique programs"
        graph_title_suffix = "\nprogram id=" + str(id) + " file=" + str(fin_path)
        plt.title(graph_title + graph_title_suffix)
        plt.xlabel('Iteration number')
        plt.ylabel('CDF')
        plt.ylim(ymin=0)
        plt.xlim(xmin=1)
        # plt.show()
        figurename = graph_title + "_" + str(id) + ".pdf"
        plt.legend()
        plt.grid()
        f.savefig(fout_path + figurename, bbox_inches='tight')
        print("draw_num_unique_progs::fout:", fout_path + figurename)
        plt.close(f)

def filter_draw_regular_graph_nodes(nodes):
    new_nodes = []
    for i, ns in enumerate(nodes):
        if len(ns) > 0:
            new_nodes.append(ns)
    return new_nodes

def gen_node_freq_map(raw_nodes):
    nodes_freq = {}
    for n in raw_nodes:
        if n in nodes_freq:
            nodes_freq[n][0] += 1
        else:
            nodes_freq[n] = [1]
    return nodes_freq

def gen_node_pos(nodes_info, nodes):
    # compute x pos for nodes and store it into node_info
    x_max = float(max([len(x) for x in nodes]))
    # print("x_max: ", x_max)
    for ns in nodes:
        for i, n in enumerate(ns):
            gap = x_max / (len(ns) + 1)
            nodes_info[n].append(float(i) * gap + gap)
    # compute y pos for nodes and store it into node_info
    for k in nodes_info.keys():
        nodes_info[k].append(k.count('0'))
    return nodes_info

def gen_node_cost(cost, max_cost, raw_nodes, nodes_info):
    dic = {}
    for i, c in enumerate(cost):
        if c >= max_cost:
            continue
        if raw_nodes[i] in dic:
            dic[raw_nodes[i]][0] += c
            dic[raw_nodes[i]][1] += 1
        else:
            dic[raw_nodes[i]] = [c, 1]
    # print("dic:", len(dic), dic)
    for k in nodes_info.keys():
        if k in dic:
            nodes_info[k].append(round(dic[k][0] / dic[k][1]))
        else:
            nodes_info[k].append(max_cost)
    return nodes_info


def add_unique_program_count(nodes_info, data):
    dic = {}
    count = data[3]
    nodes = data[4]
    for c, n in zip(count, nodes):
        if c != '1':
            continue
        if n in dic:
            dic[n] += 1
        else:
            dic[n] = 1
    for k in nodes_info.keys():
        if k in dic:
            nodes_info[k].append(dic[k])
        else:
            nodes_info[k].append(0)
    return nodes_info


# [node name]:[frequency count,
#              x pos, y pos,
#              avg legal error_cost, avg legal perf_cost, avg legal total_cost]
# if type is `prog`, add #unique programs into nodes_info
def gen_nodes_info(raw_nodes, nodes, data, type):
    nodes_info = gen_node_freq_map(raw_nodes)
    # print("nodes_info: ", nodes_info)
    # compute (x,y) pos for nodes and store it into node_info
    nodes_info = gen_node_pos(nodes_info, nodes)
    # print("nodes_info: ", nodes_info)
    max_err_cost = 100000
    # compute avg error cost (rounded) and store it into node_info
    err_cost = np.array(data[0]).astype(float).tolist()
    # print("err_cost: ", err_cost)
    gen_node_cost(err_cost, max_err_cost, raw_nodes, nodes_info)
    # print("nodes_info: ", nodes_info)
    # compute avg perf cost (rounded) and store it into node_info
    perf_cost = np.array(data[1]).astype(float).tolist()
    # print("perf_cost: ", perf_cost)
    nodes_info = gen_node_cost(perf_cost, max_err_cost, raw_nodes, nodes_info)
    # print("nodes_info: ", nodes_info)
    # compute avg total cost (rounded) and store it into node_info
    total_cost = np.array(data[2]).astype(float).tolist()
    # print("total_cost: ", total_cost)
    nodes_info = gen_node_cost(total_cost, max_err_cost, raw_nodes, nodes_info)
    # print("nodes_info: ", nodes_info)
    # TODO: compute #unique program and store it into node_info
    if type == "prog":
        nodes_info = add_unique_program_count(nodes_info, data)
    return nodes_info

# according to the fmt generated by function `gen_nodes_info`
def get_node_graph_name(node_name, info, type):
    if type == "prog":
        return (node_name + "\n" +
                str(info[0]) + " " + str(info[6]) + "\n" +
                str(info[5]) + " " + str(info[3]) + " " + str(info[4]))
    else:
        return (node_name + "\n" +
                str(info[0]) + "\n" +
                str(info[5]) + " " + str(info[3]) + " " + str(info[4]))


def get_node_pos(info):
    pos = (info[1], info[2])
    return pos

def gen_node_color(nodes_info):
    node_color = []
    for _, v in nodes_info.items():
        # nodes_color.append(pow(v[0], 0.5))
        node_color.append(v[0])
    # print("node_color: ", node_color)
    return node_color

def gen_edges_theoretical(nodes, nodes_info, type):
    edges = []
    for ns in nodes:
        for n in ns:
            v = nodes_info[n]
            n_name = get_node_graph_name(n, v, type)
            for i, b in enumerate(n):
                if b == '0':
                    x = n[:i] + '1' + n[i + 1:]
                    if x in nodes_info:
                        v = nodes_info[x]
                        x_name = get_node_graph_name(x, v, type)
                        edges.append((n_name, x_name))
    return edges

# edges_dic: [edge_name]:[edge_frequency_count]
def gen_edges_real(nodes_info, node_transfer_list, type):
    dic = {}
    n1 = node_transfer_list[0]
    for n in node_transfer_list[1:]:
        if n1 != n:
            e = (get_node_graph_name(n1, nodes_info[n1], type),
                 get_node_graph_name(n, nodes_info[n], type))
            if e in dic:
                dic[e] += 1
            else:
                dic[e] = 1
            n1 = n
    # print("dic: ", len(dic), dic)
    edges = []
    edge_color = []
    count = 0
    for i, v in dic.items():
        edges.append(i)
        edge_color.append(v)
        count += 1
    # print("edges: ", edges)
    # print("edge_color: ", edge_color)
    return edges, edge_color


# each item in `freq_text_nodes` is [text x pos, text y_pos, frequency count]
def gen_freq_text_nodes(nodes, nodes_info):
    freq_text_nodes = []
    x_pos = min([x[1] for _, x in nodes_info.items()]) - 2
    for ns in nodes:
        count = 0
        for n in ns:
            count += nodes_info[n][0]
        freq_text_nodes.append([x_pos, ns[0].count('0'), count])
    # print("freq_text_nodes: ", freq_text_nodes)
    return freq_text_nodes

def draw_transfer_graph(fin_path, fout_path, para, id, type):
    line = 0
    file_name_key = ""
    if type == "proposal":
        file_name_key = "proposal"
        line = 7
    elif type == "prog":
        file_name_key = "prog"
        line = 4
    file_name = fin_path + "raw_data_" + file_name_key + "_" + str(id) + "_" + \
                str(para[0]).rstrip('0').rstrip('.') + "_" + \
                str(para[1]).rstrip('0').rstrip('.') + ".txt"
    fout_name = fout_path + "transfer_graph_" + file_name_key + "_" + str(id) + "_" + \
                str(para[0]).rstrip('0').rstrip('.') + "_" + \
                str(para[1]).rstrip('0').rstrip('.') + ".pdf"
    graph_title = file_name_key + " transfer graph\n" + \
                 "file=" + fin_path + "\nprogramID=" + str(id) + "\n" + \
                 "w_e=" + str(para[0]).rstrip('0').rstrip('.') + "_" + \
                 "w_p=" + str(para[1]).rstrip('0').rstrip('.')
    print("  processing", file_name)
    data, _ = get_all_data_from_file(file_name)
    raw_nodes = data[line]
    # print("raw_nodes: ", raw_nodes)
    raw_nodes_ordered = sorted(list(set(raw_nodes)))
    # print("raw_nodes_ordered: ", raw_nodes_ordered)
    nodes = [[] for _ in range(len(raw_nodes[0]) + 1)]
    for n in raw_nodes_ordered:
        nodes[n.count('1')].append(n)
    # print("nodes: ", nodes)
    nodes = filter_draw_regular_graph_nodes(nodes)
    # print("nodes: ", nodes)
    # compute nodes_info, each item in info:
    # [node name]:[frequency count,
    #              x pos, y pos,
    #              avg legal error_cost, avg legal perf_cost, avg legal total_cost]
    # if type is `prog`, add #unique programs into nodes_info
    # compute frequency count and store it into node_info
    nodes_info = gen_nodes_info(raw_nodes, nodes, data, type)
    node_color = gen_node_color(nodes_info)
    # edges = gen_edges_theoretical(nodes, nodes_info)
    edges, edge_color = gen_edges_real(nodes_info, data[line], type)
    # print("edges: ", len(edges), edges)
    # print("edge_color: ", len(edge_color), edge_color)
    # draw graph
    figsize_x = max(0.35 * max(len(elem) for elem in nodes), 4)
    figsize_y = figsize_x * 0.75
    # print("figsize=", figsize_x, figsize_y)
    f = plt.figure(figsize=(figsize_x, figsize_y))
    # g = nx.Graph()
    g = nx.DiGraph()
    # add node into graph
    nodes_name = []
    for k, v in nodes_info.items():
        nodes_name.append(get_node_graph_name(k, v, type))
        g.add_node(get_node_graph_name(k, v, type), pos=get_node_pos(v))
    # print("nodes_name:", nodes_name)
    # add edge into graph
    g.add_edges_from(edges)
    pos = nx.get_node_attributes(g, 'pos')
    nx.draw(g, pos=pos, with_labels=True, node_size=350,
            font_size=3.8, node_color=node_color, cmap=plt.cm.Reds,
            edge_color='black', width=0.2, arrowstyle='->')
            # edge_color=edge_color, edge_cmap=plt.cm.Blues, width=1)

    # each item in `freq_text_nodes` is [text x pos, text y_pos, frequency count]
    freq_text_nodes = gen_freq_text_nodes(nodes, nodes_info)
    for i, n in enumerate(freq_text_nodes):
        plt.text(n[0], n[1], str(n[2]), fontsize='small')
    plt.title(graph_title)
    f.savefig(fout_name, bbox_inches='tight')
    print("fout: ", fout_name)

def draw_transfer_graph_proposal(fin_path, fout_path, parameters, prog_ids):
    for id in prog_ids:
        for para in parameters:
            draw_transfer_graph(fin_path, fout_path, para, id, "proposal")

def draw_transfer_graph_prog(fin_path, fout_path, parameters, prog_ids):
    for id in prog_ids:
        for para in parameters:
            draw_transfer_graph(fin_path, fout_path, para, id, "prog")

if __name__ == "__main__":
    fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start = parse_input(sys.argv[1:])
    print(fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start)
    draw_error_cost_funtion(fin_path, fout_path, parameters, prog_ids)
    draw_cost_funtion(fin_path, fout_path, parameters, prog_ids, best_perf_costs)
    draw_perf_cost_funtion(fin_path, fout_path, parameters, prog_ids)
    draw_smallest_perf_cost(fin_path, fout_path, parameters, prog_ids, best_perf_costs)
    draw_cost_value_of_generated_progs(fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start)
    draw_num_unique_progs(fin_path, fout_path, parameters, prog_ids)
    draw_transfer_graph_proposal(fin_path, fout_path, parameters, prog_ids)
    draw_transfer_graph_prog(fin_path, fout_path, parameters, prog_ids)
