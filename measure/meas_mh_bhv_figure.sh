# python3 <pythonfile> [parameters]
# parameters:
# -n: number of iterations
# --fin_path: input raw data path
# --bm_ids: benchmark list, e.g., "0 1"
# --best_perf_costs: best performace cost for each benchmark in `bm_ids`, 
#                    the order should keep the same as `bm_ids`
# --w_list: weight pair for error cost and performance cost, 
#           e.g., "1,0 1.5,1.5" represents error and performance cost pairs <1,0> and <1.5,1.5>
# --st_list: cost function strategy list, each item format: st_ex + st_eq + st_avg
# --st_when_to_restart_list: each item represents st_when_to_restart.
#                            MH_SAMPLER_ST_WHEN_TO_RESTART_NO_RESTART 0
#                            MH_SAMPLER_ST_WHEN_TO_RESTART_MAX_ITER 1
#                            if strategy is 0, `st_when_to_restart_niter` should be 0
#                            if strategy is 1, `st_when_to_restart_niter` can not be 0
# --st_when_to_restart_niter_list: each item represents iteration number to restart mh sampler
# --st_start_prog: st_start_prog list
#                  MH_SAMPLER_ST_NEXT_START_PROG_ORIG 0
#                  MH_SAMPLER_ST_NEXT_START_PROG_ALL_INSTS 1
#                  MH_SAMPLER_ST_NEXT_START_PROG_K_CONT_INSTS 2
# --p_list: there are three different modification typies for new proposals:  
#           modify a random instruction operand, instruction and two continuous instructions. 
#           Sum of these probabilities is 1
#           Each item represents probabilities of the first two modifications (the third can be computed).

pythonfile="measure/meas_mh_bhv_figure.py"
file_in="measure/"
file_out=${file_in}
python3 ${pythonfile} -n 100 --fin_path=${file_in} --bm_ids="0" \
--w_list="1,0" --st_list="000" --best_perf_costs="4" --steady_start=10 \
--st_when_to_restart_list="0" --st_when_to_restart_niter_list="0" \
--st_start_prog="0" --p_list="0.333333,0.333333"
