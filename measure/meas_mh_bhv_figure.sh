pythonfile="measure/meas_mh_bhv_figure.py"
file_in="measure/"
file_out=${file_in}
python3 ${pythonfile} -n 100 --fin_path=${file_in} --bm_ids="0 1" --w_list="0.5,1.5 1,1 1.5,0.5" --st_list="000 001" --best_perf_costs="4 4" --steady_start=10
