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

cost_ratios <- df$C16_MFC_Cost / df$MST_Cost

str <- args[2]

add <- function(s) {
  str <<- paste(str, s, sep = "")
}

clus_names <- c("C16", "C32", "C128")

add(" & $ \\gamma $ & -")
for (n in clus_names) {
  vars <- df[, paste(n, "_Gamma", sep = "")]
  add(paste(" & $", sprintf("%.2f", vars[[1]]), " $"))
}
add(" \\\\\n")

add(" & Cost Ratio & 1")
for (n in clus_names) {
  vars <- df[, paste(n, "_MFC_Cost", sep = "")] / df[, "MST_Cost"]
  add(paste(" & $", sprintf("%.2f", vars[[1]]), " $"))
}
add(" \\\\\n")

add("$ N=$")

add(" & Run Ratio & 1")
for (n in clus_names) {
  vars <- df[, "MST_Runtime"] / df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.2f", vars[[1]]), " $"))
}
add(" \\\\\n")

add("$")
add(n_filter)
add("$")

add(" & Run (mins) & $")
add(sprintf("%.1f", df[, "MST_Runtime"][[1]] / 1000 / 60))
add(" $")
for (n in clus_names) {
  vars <- df[, paste(n, "_MFC_Runtime", sep = "")]
  add(paste(" & $", sprintf("%.1f", vars[[1]] / 1000 / 60), " $"))
}
add(" \\\\\n")

add("\\cmidrule(lr){1-6}\n")

cat(str)
