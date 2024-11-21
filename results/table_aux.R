args <- commandArgs(trailingOnly = TRUE)

df <- read.csv(args[1])

n_filter <- max(df$N)
if (length(args) == 3) {
  n_filter <- as.numeric(args[3])
}

# print(length(args))
# print(args)
# print(n_filter)

df <- df[df$N == n_filter, ]

str <- args[2]

add <- function(s) {
  str <<- paste(str, s, sep = "")
}

clus_names <- c("C16", "C32", "C64", "C128", "C256")

add(" & $ \\gamma $ & -")
for (n in clus_names) {
  vars <- df[, paste(n, "_Gamma", sep = "")]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

add(" & Cost Ratio & 1")
for (n in clus_names) {
  vars <- df[, paste(n, "_MFC_Cost", sep = "")] / df[, "MST_Cost"]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

add("$ N=")
add(n_filter)
add("$")

add(" & Runtime Ratio & 1")
for (n in clus_names) {
  vars <- df[, "MST_Runtime"] / df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

add(" & Runtime (mins) & $")
add(sprintf("%.1f", mean(df[, "MST_Runtime"]) / 1000 / 60))
add(" \\scriptstyle{ \\pm ")
add(sprintf("%.2f", sd(df[, "MST_Runtime"]) / 1000 / 60))
add(" } $")
for (n in clus_names) {
  vars <- df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.1f", vars[[1]] / 1000 / 60), " $"))
}
add(" \\\\\n")

add(" & k-centering \\% & -")
for (n in clus_names) {
  vars <- df[, paste(n, "_Clustering_Runtime", sep = "")] / df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

add(" & Sub-MST \\% & -")
for (n in clus_names) {
  vars <- df[, paste(n, "_Sub_Clustering_Runtime", sep = "")] / df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

add(" & MFC-approx \\% & -")
for (n in clus_names) {
  vars <- df[, paste(n, "_Completion_Edges_Runtime", sep = "")] / df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
}
add(" \\\\\n")

# add(" & Corse Graph MST \\% & -")
# for (n in clus_names) {
#   vars <- df[, paste(n, "_Completion_Runtime", sep = "")] / df[, paste(n, "_MFC_Runtime", sep = "")]
#   add(paste(" & $", sprintf("%.3f", vars[[1]]), " $"))
# }
# add(" \\\\\n")


add("\\cmidrule(lr){1-8}\n")

cat(str)
