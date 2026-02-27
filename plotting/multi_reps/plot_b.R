library("ggplot2")
library(reshape2)
library(latex2exp)
library(cowplot)
library(purrr)

args <- commandArgs(trailingOnly = TRUE)

pdf(args[1], width = 6, height = 4)
df <- read.csv(args[2])

title_prefix <- args[3]

N <- 10000
# N <- 5000
c_counts <- c("C25", "C50", "C100")

# print(colnames(df))

for (c in c_counts) {
    df[[paste(c, "_Inital_Forest_Cost", sep = "")]] <- df[[paste(c, "_MFC_Cost_mu", sep = "")]] - df[[paste(c, "_MFC_Completion_Cost_mu", sep = "")]]
    df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] <- df[[paste(c, "_Find_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Pick_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Edges_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Runtime_mu", sep = "")]]
    df[[paste(c, "_MFC_Total_Runtime", sep = "")]] <- df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] + df[[paste(c, "_Sub_Clustering_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Runtime_mu", sep = "")]] + df[[paste(c, "_Clustering_Runtime_mu", sep = "")]]
    df[[paste(c, "_Alpha", sep = "")]] <- 1 + df[[paste(c, "_Reps_Cost_mu", sep = "")]] / df[[paste(c, "_Inital_Forest_Cost", sep = "")]]
}

runtime_var <- "_MFC_Runtime_mu"
# runtime_var <- "_MFC_Multi_Reps_Total_Runtime"
# runtime_var <- "_MFC_Total_Runtime"
# runtime_var <- "_Completion_Edges_Runtime_mu"
# runtime_var <- "_Sub_Clustering_Runtime_mu"

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
    df_filter[[column]]
}


formatted_plot <- function(title, x_axis, y_axis, legend_title) {
    cur <- ggplot()

    line_width <- 1

    cur <- cur + ggtitle(title) + labs(shape = legend_title, color = legend_title) +
        labs(x = x_axis, y = y_axis) + theme_bw() +
        theme(
            legend.box.background = element_rect(colour = "black", linewidth = line_width),
            legend.position = "right",
            panel.grid.major = element_blank(),
            panel.grid.minor = element_blank(),
            axis.ticks = element_line(linewidth = line_width),
            axis.ticks.length = unit(.35, "cm"),
            panel.border = element_rect(colour = "black", fill = NA, linewidth = line_width),
            plot.margin = grid::unit(c(10, 1, 0, 3), "mm")
        ) +
        guides(color = guide_legend(override.aes = list(size = 5)))

    return(cur)
}

generate_plot <- function(
    title,
    x_var,
    y_var,
    x_axis,
    y_axis,
    show_lines = TRUE,
    show_fr = TRUE,
    show_opt = TRUE,
    greedy_color = "Greedy", greedy_shape = "Greedy",
    dp_color = "DP", dp_shape = "DP",
    fr_color = "FR", fr_shape = "FR") {
    opt_val <- df[df$N == N & df$RunType == " opt", ][[1, y_var]]

    cur <- formatted_plot(title, x_axis, y_axis, "Type")

    cur <- cur + geom_point(data = data.frame(x = extract_df_greedy(x_var), y = extract_df_greedy(y_var)), aes(x = x, y = y, color = greedy_color, shape = greedy_shape))
    cur <- cur + geom_point(data = data.frame(x = extract_df_dp(x_var), y = extract_df_dp(y_var)), aes(x = x, y = y, color = dp_color, shape = dp_shape))

    if (show_opt) {
        cur <- cur + geom_point(data = data.frame(x = df[df$N == N & df$RunType == " opt", ][[1, x_var]], y = df[df$N == N & df$RunType == " opt", ][[1, y_var]]), aes(x = x, y = y, color = "Opt", shape = "Opt"))
    }

    if (show_fr) {
        cur <- cur + geom_point(data = data.frame(x = extract_df_fr(x_var), y = extract_df_fr(y_var)), aes(x = x, y = y, color = fr_color, shape = fr_shape))
    }

    if (show_lines) {
        cur <- cur + geom_hline(yintercept = opt_val, color = "green")
        cur <- cur + geom_text(aes(1, opt_val, label = "opt", vjust = -1), color = "green", size = 2)
    }

    return(cur)
}

plot_one <- function(title, x_var, y_var, x_axis, y_axis, show_lines = TRUE, show_opt = TRUE) {
    print(generate_plot(title, x_var, y_var, x_axis, y_axis, show_lines, show_opt))
}

plot_alpha <- function(title_prefix, c) {
    cur <- generate_plot(
        paste(title_prefix, paste(c, "Runtime vs Cost Ratio")),
        paste(c, runtime_var, sep = ""),
        paste(c, "_Alpha", sep = ""),
        "Runtime",
        "Cost Ratio",
        show_lines = FALSE,
        show_fr = TRUE,
        greedy_color = "Greedy Alpha", greedy_shape = "Greedy",
        dp_color = "DP Alpha", dp_shape = "DP",
        fr_color = "FR Alpha", fr_shape = "FR"
    )

    opt_val <- df[df$N == N & df$RunType == " opt", ][[1, paste(c, "_MFC_Cost_mu", sep = "")]]

    greedy_real_val <- extract_df_greedy(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    dp_real_val <- extract_df_dp(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    fr_real_val <- extract_df_fr(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val

    cur <- cur + geom_point(data = data.frame(x = extract_df_greedy(paste(c, runtime_var, sep = "")), y = greedy_real_val), aes(x = x, y = y, color = "Greedy", shape = "Greedy"))
    cur <- cur + geom_point(data = data.frame(x = extract_df_dp(paste(c, runtime_var, sep = "")), y = dp_real_val), aes(x = x, y = y, color = "DP", shape = "DP"))
    cur <- cur + geom_point(data = data.frame(x = extract_df_fr(paste(c, runtime_var, sep = "")), y = fr_real_val), aes(x = x, y = y, color = "FR", shape = "FR"))

    print(cur)
}

plot_additive <- function(title_prefix, c) {
    cur <- formatted_plot(paste(title_prefix, paste(c, "Runtime Vs Additive Error")), "Runtime", "Additive Error", "Type")

    df_opt <- df[df$N == N & df$RunType == " opt", ]

    opt_val <- df[df$N == N & df$RunType == " opt", ][[1, paste(c, "_MFC_Cost_mu", sep = "")]]

    cur <- cur + geom_point(
        data = data.frame(
            x = df_opt[[1, paste(c, runtime_var, sep = "")]], 
            y = df_opt[[1, paste(c, "_MFC_Cost_mu", sep = "")]] - opt_val
        ), 
        aes(x = x, y = y, color = "Opt", shape = "Opt")
    )

    cur <- cur + geom_point(
        data = data.frame(
            x = extract_df_greedy(paste(c, runtime_var, sep = "")),
            y = extract_df_greedy(paste(c, "_MFC_Cost_mu", sep = "")) - opt_val
        ),
        aes(x = x, y = y, color = "Greedy", shape = "Greedy")
    )

    cur <- cur + geom_point(
        data = data.frame(
            x = extract_df_dp(paste(c, runtime_var, sep = "")),
            y = extract_df_dp(paste(c, "_MFC_Cost_mu", sep = "")) - opt_val
        ),
        aes(x = x, y = y, color = "DP", shape = "DP")
    )

    cur <- cur + geom_point(
        data = data.frame(
            x = extract_df_fr(paste(c, runtime_var, sep = "")),
            y = extract_df_fr(paste(c, "_MFC_Cost_mu", sep = "")) - opt_val
        ),
        aes(x = x, y = y, color = "Fixed Reps", shape = "Fixed Reps")
    )

    print(cur)
}

plot_cost_ratio <- function(title_prefix, c) {
    cur <- formatted_plot(paste(title_prefix, paste(c, "Rep Count vs Cost Ratio")), "Rep Count", "Cost Ratio", "Type")

    df_opt <- df[df$N == N & df$RunType == " opt", ]
    opt_val <- df_opt[[1, paste(c, "_MFC_Cost_mu", sep = "")]]

    greedy_real_val <- extract_df_greedy(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    dp_real_val <- extract_df_dp(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    fr_real_val <- extract_df_fr(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val

    # cur <- cur + geom_point(data = data.frame(x = df_opt[[1, paste(c, "_Rep_Count_mu", sep = "")]], y = df_opt[[1, paste(c, "_MFC_Cost_mu", sep = "")]] / opt_val), aes(x = x, y = y, color = "Opt", shape = "Opt"))

    tmp <- cur + geom_point(data = data.frame(x = extract_df_greedy(paste(c, "_Rep_Count_mu", sep = "")), y = extract_df_greedy(paste(c, "_Alpha", sep = ""))), aes(x = x, y = y, color = "Greedy Alpha", shape = "Greedy Alpha"))
    tmp <- tmp + geom_point(data = data.frame(x = extract_df_greedy(paste(c, "_Rep_Count_mu", sep = "")), y = greedy_real_val), aes(x = x, y = y, color = "Greedy", shape = "Greedy"))
    print(tmp)

    tmp <- cur + geom_point(data = data.frame(x = extract_df_dp(paste(c, "_Rep_Count_mu", sep = "")), y = extract_df_dp(paste(c, "_Alpha", sep = ""))), aes(x = x, y = y, color = "DP Alpha", shape = "DP Alpha"))
    tmp <- tmp + geom_point(data = data.frame(x = extract_df_dp(paste(c, "_Rep_Count_mu", sep = "")), y = dp_real_val), aes(x = x, y = y, color = "DP", shape = "DP"))
    print(tmp)

    tmp <- cur + geom_point(data = data.frame(x = extract_df_fr(paste(c, "_Rep_Count_mu", sep = "")), y = extract_df_fr(paste(c, "_Alpha", sep = ""))), aes(x = x, y = y, color = "FR Alpha", shape = "FR Alpha"))
    tmp <- tmp + geom_point(data = data.frame(x = extract_df_fr(paste(c, "_Rep_Count_mu", sep = "")), y = fr_real_val), aes(x = x, y = y, color = "FR", shape = "FR"))
    print(tmp)
}

for (c in c_counts) {
    plot_alpha(title_prefix, c)
    plot_cost_ratio(title_prefix, c)
    plot_additive(title_prefix, c)
    plot_one(paste(title_prefix, paste(c, "Runtime vs MFC Edges Cost")), paste(c, runtime_var, sep = ""), paste(c, "_MFC_Cost_mu", sep = ""), "Runtime", "MFC Cost", show_lines = FALSE)
    plot_one(paste(title_prefix, paste(c, "Rep Count vs MFC Edges Cost")), paste(c, "_Rep_Count_mu", sep = ""), paste(c, "_MFC_Cost_mu", sep = ""), "Rep Count", "MFC Cost", show_lines = FALSE, show_opt = FALSE)
}
