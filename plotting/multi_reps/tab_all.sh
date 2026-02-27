echo '\\resizebox{\\textwidth}{!}{'
echo '\\begin{tabular}{l cc ccccc ccccc ccccc l}'
echo '\\multirow{2}{*}{Dataset} & \\multirow{2}{*}{Alg}'
echo '& \multicolumn{5}{c}{$b=0$}'
echo '& \multicolumn{5}{c}{$b=t$}'
echo '& \multicolumn{5}{c}{$b=2t$}'
echo '\\\\'
echo '\\cmidrule(lr){3-7}'
echo '\\cmidrule(lr){8-12}'
echo '\\cmidrule(lr){13-17}'
echo '& '
echo '& $\\varepsilon$ & $\\varepsilon_\\alpha$ & Comp & DC & RT'
echo '& $\\varepsilon$ & $\\varepsilon_\\alpha$ & Comp & DC & RT'
echo '& $\\varepsilon$ & $\\varepsilon_\\alpha$ & Comp & DC & RT'
echo '\\\\'
echo '\\toprule'

Rscript tab_paper.R ${1}/jaccard_cooking_0.txt Cooking 39774 
echo '\\midrule'
Rscript tab_paper.R ${1}/hdf5_fashion.txt Fashion 30000 
echo '\\midrule'
Rscript tab_paper.R ${1}/edit_distance_names_us.txt Names 30000 
echo '\\midrule'
Rscript tab_paper.R ${1}/hamming_gg.txt GG 30000 
echo '\\bottomrule'

echo '\\end{tabular}'
echo '}'
