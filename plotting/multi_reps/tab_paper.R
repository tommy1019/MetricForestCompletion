library(purrr, exclude = "discard")

args <- commandArgs(trailingOnly = TRUE)

df <- read.csv(args[1])
title <- args[2]

N <- as.numeric(args[3])

c_nums <- c(floor(sqrt(N)))

c_counts <- map(c_nums, function(c) {
    paste("C", c, sep = "")
})

for (c_num in c_nums) {
    c <- paste("C", c_num, sep = "")
    df[[paste(c, "_B", sep = "")]] <- df[[paste(c, "_Rep_Count_mu", sep = "")]] - c_num
}

# Workaround for DP no calculating correct cost for b=0
for (c_num in c_nums) {
    c <- paste("C", c_num, sep = "")

    df[df$N == N & startsWith(df$RunType, " dp_") & df[[paste(c, "_B", sep = "")]] == 0, paste(c, "_Reps_Cost_mu", sep = "")] <- df[df$N == N & startsWith(df$RunType, " greedy_") & df[[paste(c, "_B", sep = "")]] == 0, paste(c, "_Reps_Cost_mu", sep = "")]
}

# Generate new columns for additional stats
for (c_num in c_nums) {
    c <- paste("C", c_num, sep = "")

    df[[paste(c, "_Inital_Forest_Cost", sep = "")]] <- df[[paste(c, "_MFC_Cost_mu", sep = "")]] - df[[paste(c, "_MFC_Completion_Cost_mu", sep = "")]]
    df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] <- df[[paste(c, "_Find_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Pick_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Edges_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Runtime_mu", sep = "")]]
    df[[paste(c, "_MFC_Total_Runtime", sep = "")]] <- df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] + df[[paste(c, "_Sub_Clustering_Runtime_mu", sep = "")]] + df[[paste(c, "_Clustering_Runtime_mu", sep = "")]]
    df[[paste(c, "_Alpha", sep = "")]] <- 1 + df[[paste(c, "_Reps_Cost_mu", sep = "")]] / df[[paste(c, "_Inital_Forest_Cost", sep = "")]]

    df$RunTypeNum <- as.numeric(substring(df$RunType, 13))
}

# Reorder by FR budget
df <- df[order(df$RunTypeNum), ]

# Extract the opt row
df_opt <- df[df$N == N & df$RunType == " opt", ]

# extract functions for dp, greedy, and fr
extract_df_dp <- function(column) {
    df_filter <- df[df$N == N, ]
    df_filter <- df_filter[startsWith(df$RunType, " dp_"), ]
    df_filter[[column]]
}
extract_df_greedy <- function(column) {
    df_filter <- df[df$N == N, ]
    df_filter <- df_filter[startsWith(df$RunType, " greedy_"), ]
    df_filter[[column]]
}
extract_df_fr <- function(column) {
    df_filter <- df[df$N == N, ]
    df_filter <- df_filter[startsWith(df$RunType, " fixed_reps_"), ]
    head(df_filter[[column]], -1)
}

# Main plotting function, generates a plot based on c the number of clusters and exports it
plot_alpha <- function(title_prefix, c) {
    y_var_alpha <- paste(c, "_Alpha", sep = "")

    opt_cost <- df_opt[[1, paste(c, "_MFC_Cost_mu", sep = "")]]
    opt_mfc_cost <- df_opt[[1, paste(c, "_MFC_Completion_Cost_mu", sep = "")]]

    greedy_cost_ratio <- extract_df_greedy(paste(c, "_MFC_Cost_mu", sep = "")) / opt_cost
    dp_cost_ratio <- extract_df_dp(paste(c, "_MFC_Cost_mu", sep = "")) / opt_cost
    fr_cost_ratio <- extract_df_fr(paste(c, "_MFC_Cost_mu", sep = "")) / opt_cost

    greedy_cost_alpha <- extract_df_greedy(paste(c, "_Alpha", sep = ""))
    dp_cost_alpha <- extract_df_dp(paste(c, "_Alpha", sep = ""))
    fr_cost_alpha <- extract_df_fr(paste(c, "_Alpha", sep = ""))

    greedy_mfc_cost_ratio <- extract_df_greedy(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_mfc_cost
    dp_mfc_cost_ratio <- extract_df_dp(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_mfc_cost
    fr_mfc_cost_ratio <- extract_df_fr(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_mfc_cost

    opt_runtime <- df_opt[[1, paste(c, "_MFC_Runtime_mu", sep = "")]] / 1000

    greedy_b <- extract_df_greedy(paste(c, "_B", sep = ""))
    dp_b <- extract_df_dp(paste(c, "_B", sep = ""))
    fr_b <- extract_df_fr(paste(c, "_B", sep = ""))

    greedy_runtime <- extract_df_greedy(paste(c, "_MFC_Runtime_mu", sep = ""))
    dp_runtime <- extract_df_dp(paste(c, "_MFC_Runtime_mu", sep = ""))
    fr_runtime <- extract_df_fr(paste(c, "_MFC_Runtime_mu", sep = ""))

    greedy_dc <- extract_df_greedy(paste(c, "_Dist_Calls_mu", sep = ""))
    dp_dc <- extract_df_dp(paste(c, "_Dist_Calls_mu", sep = ""))
    fr_dc <- extract_df_fr(paste(c, "_Dist_Calls_mu", sep = ""))

    b_vals <- c(fr_b[[1]], fr_b[[2]], fr_b[[3]])

    print_row <- function(title, b_idxs, cost_ratio, alpha, mfc_cost_ratio, dc, runtime) {
        cat(title)
        for (b in b_vals) {
            i <- which(b_idxs == b)
            cat(sprintf(" & %.4f & %.4f & %0.2f & %.1f & %.1f", cost_ratio[i] - 1, alpha[i] - 1, mfc_cost_ratio[i], dc[i] / 1000000, runtime[i] / 1000.0))
        }
        cat(" \\\\ \n")
    }

    cat(paste0("\\multirow{3}{*}{", title, "} & "))
    print_row("DP", dp_b, dp_cost_ratio, dp_cost_alpha, dp_mfc_cost_ratio, dp_dc, dp_runtime)
    cat(" & ")
    print_row("Greedy", greedy_b, greedy_cost_ratio, greedy_cost_alpha, greedy_mfc_cost_ratio, greedy_dc, greedy_runtime)
    cat(" & ")
    print_row("Fixed", fr_b, fr_cost_ratio, fr_cost_alpha, fr_mfc_cost_ratio, fr_dc, fr_runtime)
}

# Loop over all c and generates plots
for (c in c_counts) {
    plot_alpha(title_prefix, c)
}
