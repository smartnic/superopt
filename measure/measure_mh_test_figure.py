#!/usr/bin/python3
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm
import sys

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
    parameters = [[float(parameters[i]), float(parameters[i+1])] for i in range(0, len(parameters), 2)]
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
            y_axis.append(y_axis[i]+1)
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
        figurename = graph_title + "_" + str(id) +".pdf"
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
            fig_x_axis = [x/best_perf_cost for x in fig_x_axis]
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
            x_axis = list(range(1, num_iter+1, 1))
            # smalles_perf_cost
            y_axis = []
            if error_cost[0] == 0:
                y_axis.append(perf_cost[0])
            else:
                y_axis.append(14)   # max_perf_cost
            n = 10
            for i, c in enumerate(perf_cost):
                if i != 0:
                    if error_cost[i] == 0:
                        if c < y_axis[i-1]:
                            n = i + 500
                        y_axis.append(min(c, y_axis[i-1]))
                    else:
                        y_axis.append(y_axis[i-1])
            plt.plot(x_axis[:n], y_axis[:n], linestyle='-.', linewidth=1.5, label=curve_name)
        graph_title = "Relationship between smallest performance cost and iteration number"
        graph_title_suffix = "\nprogram id=" + str(id) + " error cost=0\n optimal performance cost="+str(best_c)+\
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
    # print(len(cost_list), cost_list)
    i = 0
    x_axis = [cost_list[0]]
    y_axis = [1]
    for c in cost_list[1:]:
        if c == x_axis[i]:
            y_axis[i] += 1
        else:
            x_axis.append(c)
            y_axis.append(y_axis[i]+1)
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
        graph_title_suffix = "\nprogram id=" + str(id)
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

if __name__=="__main__":
    fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start = parse_input(sys.argv[1:])
    print(fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start)
    draw_error_cost_funtion(fin_path, fout_path, parameters, prog_ids)
    draw_cost_funtion(fin_path, fout_path, parameters, prog_ids, best_perf_costs)
    draw_perf_cost_funtion(fin_path, fout_path, parameters, prog_ids)
    draw_smallest_perf_cost(fin_path, fout_path, parameters, prog_ids, best_perf_costs)
    draw_cost_value_of_generated_progs(fin_path, fout_path, parameters, prog_ids, best_perf_costs, steady_start)
