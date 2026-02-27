# Generate all plots for paper

Rscript plot_paper.R jaccard_cooking_0_cr.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 cr no no ylog 39774 max &
Rscript plot_paper.R hdf5_fashion_cr.pdf ${1}/hdf5_fashion.txt hdf5_fashion cr no no ylog 30000 max &
Rscript plot_paper.R edit_distance_names_us_cr.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us cr no no ylog 30000 max &
Rscript plot_paper.R hamming_gg_cr.pdf ${1}/hamming_gg.txt hamming_gg cr no no ylog 30000 max &


Rscript plot_paper.R jaccard_cooking_0_alpha.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 alpha no no ylog 39774 max &
Rscript plot_paper.R hdf5_fashion_alpha.pdf ${1}/hdf5_fashion.txt hdf5_fashion alpha no no ylog 30000 max &
Rscript plot_paper.R edit_distance_names_us_alpha.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us alpha no no ylog 30000 max &
Rscript plot_paper.R hamming_gg_alpha.pdf ${1}/hamming_gg.txt hamming_gg alpha no no ylog 30000 max &


Rscript plot_paper.R jaccard_cooking_0_diff.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 diff no yes ylog 39774 max &
Rscript plot_paper.R hdf5_fashion_diff.pdf ${1}/hdf5_fashion.txt hdf5_fashion diff no yes ylog 30000 max &
Rscript plot_paper.R edit_distance_names_us_diff.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us diff no yes ylog 30000 max &
Rscript plot_paper.R hamming_gg_diff.pdf ${1}/hamming_gg.txt hamming_gg diff no yes ylog 30000 max &




Rscript plot_paper.R jaccard_cooking_0_alpha_x_lim.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 alpha no no ylog 39774 80 500 &
Rscript plot_paper.R hdf5_fashion_alpha_x_lim.pdf ${1}/hdf5_fashion.txt hdf5_fashion alpha no no ylog 30000 50 200 &
Rscript plot_paper.R edit_distance_names_us_alpha_x_lim.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us alpha no no ylog 30000 85 105 &
Rscript plot_paper.R hamming_gg_alpha_x_lim.pdf ${1}/hamming_gg.txt hamming_gg alpha no no ylog 30000 300 900 &









Rscript plot_paper.R b_jaccard_cooking_0_cr.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 cr budget no ylog 39774 max &
Rscript plot_paper.R b_hdf5_fashion_cr.pdf ${1}/hdf5_fashion.txt hdf5_fashion cr budget no ylog 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_cr.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us cr budget no ylog 30000 max &
Rscript plot_paper.R b_hamming_gg_cr.pdf ${1}/hamming_gg.txt hamming_gg cr budget no ylog 30000 max &


Rscript plot_paper.R b_jaccard_cooking_0_alpha.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 alpha budget no ylog 39774 max &
Rscript plot_paper.R b_hdf5_fashion_alpha.pdf ${1}/hdf5_fashion.txt hdf5_fashion alpha budget no ylog 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_alpha.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us alpha budget no ylog 30000 max &
Rscript plot_paper.R b_hamming_gg_alpha.pdf ${1}/hamming_gg.txt hamming_gg alpha budget no ylog 30000 max &


Rscript plot_paper.R b_jaccard_cooking_0_diff.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 diff budget yes ylog 39774 max &
Rscript plot_paper.R b_hdf5_fashion_diff.pdf ${1}/hdf5_fashion.txt hdf5_fashion diff budget yes ylog 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_diff.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us diff budget yes ylog 30000 max &
Rscript plot_paper.R b_hamming_gg_diff.pdf ${1}/hamming_gg.txt hamming_gg diff budget yes ylog 30000 max &


Rscript plot_paper.R b_jaccard_cooking_0_runtime.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 runtime budget yes ynorm 39774 max &
Rscript plot_paper.R b_hdf5_fashion_runtime.pdf ${1}/hdf5_fashion.txt hdf5_fashion runtime budget yes ynorm 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_runtime.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us runtime budget yes ynorm 30000 max &
Rscript plot_paper.R b_hamming_gg_runtime.pdf ${1}/hamming_gg.txt hamming_gg runtime budget yes ynorm 30000 max &


Rscript plot_paper.R b_jaccard_cooking_0_mfc_cr.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 mfc_cr budget yes ynorm 39774 max &
Rscript plot_paper.R b_hdf5_fashion_mfc_cr.pdf ${1}/hdf5_fashion.txt hdf5_fashion mfc_cr budget yes ynorm 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_mfc_cr.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us mfc_cr budget yes ynorm 30000 max &
Rscript plot_paper.R b_hamming_gg_mfc_cr.pdf ${1}/hamming_gg.txt hamming_gg mfc_cr budget yes ynorm 30000 max &


Rscript plot_paper.R b_jaccard_cooking_0_dist.pdf ${1}/jaccard_cooking_0.txt jaccard_cooking_0 distc budget yes ylog 39774 max &
Rscript plot_paper.R b_hdf5_fashion_dist.pdf ${1}/hdf5_fashion.txt hdf5_fashion distc budget yes ylog 30000 max &
Rscript plot_paper.R b_edit_distance_names_us_dist.pdf ${1}/edit_distance_names_us.txt edit_distance_names_us distc budget yes ylog 30000 max &
Rscript plot_paper.R b_hamming_gg_dist.pdf ${1}/hamming_gg.txt hamming_gg distc budget yes ylog 30000 max &


wait

