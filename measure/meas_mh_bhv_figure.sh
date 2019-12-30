pythonfile="measure/meas_mh_bhv_figure.py"
file_in="measure/"
file_out=${file_in}
python3 ${pythonfile} -n 100 --fin_path=${file_in} --bm_ids="0" \
--w_list="1,0" --st_list="000" --best_perf_costs="4" --steady_start=10 \
--st_when_to_restart_list="0" --st_when_to_restart_niter_list="0" \
--st_start_prog="0" --p_list="0.333333,0.333333"
