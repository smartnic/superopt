# ./measure_mh_test.out 10000 0.45 1 ../superopt_figure/data/cost_sample1.dat
# ./measure_mh_test.out 10000 0.45 2 ../superopt_figure/data/cost_sample2.dat
# ./measure_mh_test.out 10000 0.45 3 ../superopt_figure/data/cost_sample3.dat
path="../superopt_figure/data/"
./measure_mh_test.out 10000 0.45 0.55 ${path}cost_sample2-1.dat ${path}best_iternum2-1.dat 3
./measure_mh_test.out 10000 0.9 0.1  ${path}cost_sample2-2.dat ${path}best_iternum2-2.dat 3
# ./measure_mh_test.out 10000 0.45 2 ${path}cost_sample1.dat ${path}best_iternum1.dat 1
# ./measure_mh_test.out 10000 0.45 2 ${path}cost_sample1.dat ${path}best_iternum2.dat 2
