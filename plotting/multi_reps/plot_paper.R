library(ggplot2)
library(latex2exp)
library(scales)
library(purrr, exclude = "discard")

# Plotting code for the multi_reps paper

args <- commandArgs(trailingOnly = TRUE)

# Width and height of the pdf
width <- 2.5
height <- 1.7

if (args[6] != "no") {
    height <- height + 0.25
}
# if (args[7] != "no") {
#     width <- width + 0.25
# }

pdf(args[1], width = width, height = height)

# Read dataset in
df <- read.csv(args[2])

# Title prefix - unused
title_prefix <- args[3]

hide_legend <- TRUE
add_lines <- TRUE

N <- as.numeric(args[8])

c_nums <- c(25, 50, 100)
c_nums <- c(floor(sqrt(N)))

c_counts <- map(c_nums, function(c) {
    paste("C", c, sep = "")
})

# Generate new columns for additional stats that we can plot
for (c_num in c_nums) {
    c <- paste("C", c_num, sep = "")

    if (df[df$RunType == " dp_1.000000", ][1, paste0(c, "_Reps_Cost_mu")] == 0) {
        r <- df[df$RunType == " fixed_reps_1", ][1, paste0(c, "_Reps_Cost_mu")]
        print(paste0("Found dp_1.000000 bug... replacing with fixed_reps_1 data: ", r))
        df[which(df$RunType == " dp_1.000000")[1], paste0(c, "_Reps_Cost_mu")] <- r
    }

    df[[paste(c, "_Inital_Forest_Cost", sep = "")]] <- df[[paste(c, "_MFC_Cost_mu", sep = "")]] - df[[paste(c, "_MFC_Completion_Cost_mu", sep = "")]]
    df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] <- df[[paste(c, "_Find_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Pick_Reps_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Edges_Runtime_mu", sep = "")]] + df[[paste(c, "_Completion_Runtime_mu", sep = "")]]
    df[[paste(c, "_MFC_Total_Runtime", sep = "")]] <- df[[paste(c, "_MFC_Multi_Reps_Total_Runtime", sep = "")]] + df[[paste(c, "_Sub_Clustering_Runtime_mu", sep = "")]] + df[[paste(c, "_Clustering_Runtime_mu", sep = "")]]
    df[[paste(c, "_Alpha", sep = "")]] <- 1 + df[[paste(c, "_Reps_Cost_mu", sep = "")]] / df[[paste(c, "_Inital_Forest_Cost", sep = "")]]
    df[[paste(c, "_B", sep = "")]] <- df[[paste(c, "_Rep_Count_mu", sep = "")]] - c_num

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
    df_filter[[column]]
    # head(df_filter[[column]], -1)
}

# Creates a ggplot formatted for the paper
formatted_plot <- function(title, x_axis, y_axis) {
    cur <- ggplot()

    line_width <- 1
    text_size <- 15

    legend_title <- "Type"

    cur <- cur + ggtitle(title) + labs(color = legend_title) +
        labs(x = x_axis, y = y_axis) + theme_bw() +
        theme(
            text = element_text(size = text_size),
            legend.box.background = element_rect(colour = "black", linewidth = line_width),
            legend.position = "right",
            panel.grid.major = element_blank(),
            panel.grid.minor = element_blank(),
            axis.ticks = element_line(linewidth = line_width / 2),
            axis.ticks.length = unit(.15, "cm"),
            panel.border = element_rect(colour = "black", fill = NA, linewidth = line_width),
            plot.margin = grid::unit(c(4.5, 2.5, 1, 1), "mm"),
            axis.text.y = element_text(hjust = 0)
        ) +
        guides(color = guide_legend(override.aes = list(size = 5))) +
        labs(title = NULL) +
        guides(x = guide_axis(check.overlap = TRUE))


    if (hide_legend) {
        cur <- cur + theme(legend.position = "none")
    }

    return(cur)
}

# Main plotting function, generates a plot based on c the number of clusters and exports it
plot_alpha <- function(title_prefix, c) {
    # Customization functions
    x_budget <- args[5] == "budget"
    x_runtime_ratio <- FALSE

    y_translate <- -1 + .Machine$double.eps
    y_log <- args[7] == "ylog"

    plot_opt <- args[5] != "budget"
    plot_b_0 <- !x_budget

    plot_cost_ratio <- args[4] == "cr"
    plot_alpha <- args[4] == "alpha"
    plot_diff <- args[4] == "diff"
    plot_runtime <- args[4] == "runtime"
    plot_mfc_cr <- args[4] == "mfc_cr"
    plot_distc <- args[4] == "distc"

    runtime_var <- "_MFC_Runtime_mu"

    x_lab <- "Runtime (s)"
    y_lab <- TeX("$\\epsilon$ = Cost Ratio - $1_{\\phantom{a}}$")

    # Select correct y_lab based on which plots are being plotted
    if (plot_alpha) {
        y_lab <- TeX("$\\epsilon_\\alpha = \\alpha - 1$")
    }
    if (plot_diff) {
        y_lab <- TeX("$\\alpha_{\\phantom{a}}$ - Cost Ratio")
    }
    if (x_runtime_ratio) {
        x_lab <- "Runtime Ratio"
        runtime_var <- "_MFC_Multi_Reps_Total_Runtime"
    }
    if (plot_runtime) {
        y_lab <- TeX("Runtime")
    }
    if (plot_mfc_cr) {
        y_lab <- TeX("Completion Ratio")
        y_translate <- 0
    }
    if (plot_distc) {
        y_lab <- TeX("Distance Calls")
        y_translate <- 0
    }

    x_var <- paste(c, runtime_var, sep = "")
    x_var_scale <- 1000

    y_var_alpha <- paste(c, "_Alpha", sep = "")

    point_size <- 0.9

    alpha_lab <- " Alpha"
    alpha_lab <- ""

    opt_val <- df_opt[[1, paste(c, "_MFC_Cost_mu", sep = "")]]

    # Extract cost ratios
    greedy_real_val <- extract_df_greedy(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    dp_real_val <- extract_df_dp(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val
    fr_real_val <- extract_df_fr(paste(c, "_MFC_Cost_mu", sep = "")) / opt_val

    if (plot_mfc_cr) {
        print(y_translate)
        opt_val <- df_opt[[1, paste(c, "_MFC_Completion_Cost_mu", sep = "")]]

        greedy_real_val <- extract_df_greedy(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_val
        dp_real_val <- extract_df_dp(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_val
        fr_real_val <- extract_df_fr(paste(c, "_MFC_Completion_Cost_mu", sep = "")) / opt_val
    }

    if (x_budget) {
        x_lab <- "b"
        show_opt <- FALSE
        x_var <- paste(c, "_B", sep = "")
        x_var_scale <- 1
    }

    opt_x <- df_opt[[1, x_var]] / 1000

    # Rescale x axis. Used for runtime to go from milliseconds to seconds
    greedy_x <- extract_df_greedy(x_var) / x_var_scale
    dp_x <- extract_df_dp(x_var) / x_var_scale
    fr_x <- extract_df_fr(x_var) / x_var_scale

    # Rescale x axis by opt if plotting runtime ratios
    if (x_runtime_ratio & !x_budget) {
        greedy_x <- greedy_x / opt_x
        dp_x <- dp_x / opt_x
        fr_x <- fr_x / opt_x

        opt_x <- 1
    }

    # generate base ggplot
    cur <- formatted_plot(
        paste(title_prefix, paste(c, "Runtime vs Cost Ratio")),
        x_lab,
        y_lab
    )

    # Helper function to get y level to put opt and b=0 text
    get_text_y <- function(f, inital) {
        opt_text_y <- inital

        if (plot_alpha) {
            opt_text_y <- f(opt_text_y, f(extract_df_greedy(y_var_alpha) + y_translate))
            opt_text_y <- f(opt_text_y, f(extract_df_dp(y_var_alpha) + y_translate))
            opt_text_y <- f(opt_text_y, f(extract_df_fr(y_var_alpha) + y_translate))
        }
        if (plot_cost_ratio || plot_mfc_cr) {
            opt_text_y <- f(opt_text_y, f(greedy_real_val + y_translate))
            opt_text_y <- f(opt_text_y, f(dp_real_val + y_translate))
            opt_text_y <- f(opt_text_y, f(fr_real_val + y_translate))
        }
        if (plot_diff) {
            opt_text_y <- f(opt_text_y, f(extract_df_greedy(y_var_alpha) - greedy_real_val))
            opt_text_y <- f(opt_text_y, f(extract_df_dp(y_var_alpha) - dp_real_val))
            opt_text_y <- f(opt_text_y, f(extract_df_fr(y_var_alpha) - fr_real_val))
        }

        opt_text_y
    }

    # Add opt v-line and text
    if (plot_opt) {
        inital <- 0
        f <- max
        hjust <- 1

        if (title_prefix == "edit_distance_names_us") {
            inital <- 1
            f <- min
            hjust <- 0
        }

        cur <- cur + geom_vline(xintercept = opt_x, linetype = "dashed", color = "magenta")
        cur <- cur + annotate("text",
            x = opt_x,
            y = get_text_y(f, inital),
            label = "opt",
            angle = 90,
            vjust = -0.5,
            hjust = hjust,
            color = "magenta"
        )
    }

    # Add b=0 v-line
    if (plot_b_0) {
        v <- min(greedy_x)
        cur <- cur + geom_vline(xintercept = v, linetype = "dashed", color = "magenta")
        cur <- cur + annotate("text",
            x = v,
            y = get_text_y(min, 1),
            label = "b=0",
            angle = 90,
            vjust = 1.3,
            hjust = 0,
            color = "magenta"
        )
    }

    # Add lines if we want lines connecting the dots together
    if (add_lines) {
        if (plot_alpha) {
            cur <- cur + geom_line(data = data.frame(x = greedy_x, y = extract_df_greedy(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = dp_x, y = extract_df_dp(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = fr_x, y = extract_df_fr(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
        }

        if (plot_cost_ratio || plot_mfc_cr) {
            cur <- cur + geom_line(data = data.frame(x = greedy_x, y = greedy_real_val + y_translate), aes(x = x, y = y, color = "Greedy"))
            cur <- cur + geom_line(data = data.frame(x = dp_x, y = dp_real_val + y_translate), aes(x = x, y = y, color = "DP"))
            cur <- cur + geom_line(data = data.frame(x = fr_x, y = fr_real_val + y_translate), aes(x = x, y = y, color = "FR"))
        }

        if (plot_diff) {
            cur <- cur + geom_line(data = data.frame(x = greedy_x, y = extract_df_greedy(y_var_alpha) - greedy_real_val), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = dp_x, y = extract_df_dp(y_var_alpha) - dp_real_val), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = fr_x, y = extract_df_fr(y_var_alpha) - fr_real_val), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
        }

        if (plot_runtime) {
            cur <- cur + geom_line(data = data.frame(x = greedy_x, y = extract_df_greedy(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = dp_x, y = extract_df_dp(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = fr_x, y = extract_df_fr(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
        }

        if (plot_distc) {
            cur <- cur + geom_line(data = data.frame(x = greedy_x, y = extract_df_greedy(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = dp_x, y = extract_df_dp(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
            cur <- cur + geom_line(data = data.frame(x = fr_x, y = extract_df_fr(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
        }
    }

    # Add dots if we are plotting those values
    if (plot_alpha) {
        cur <- cur + geom_point(data = data.frame(x = greedy_x, y = extract_df_greedy(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")), shape = 0, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = dp_x, y = extract_df_dp(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")), shape = 1, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = fr_x, y = extract_df_fr(y_var_alpha) + y_translate), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")), shape = 2, size = point_size)
    }

    if (plot_cost_ratio || plot_mfc_cr) {
        cur <- cur + geom_point(data = data.frame(x = greedy_x, y = greedy_real_val + y_translate), aes(x = x, y = y, color = "Greedy"), shape = 0, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = dp_x, y = dp_real_val + y_translate), aes(x = x, y = y, color = "DP"), shape = 1, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = fr_x, y = fr_real_val + y_translate), aes(x = x, y = y, color = "FR"), shape = 1, size = point_size)
    }

    if (plot_diff) {
        cur <- cur + geom_point(data = data.frame(x = greedy_x, y = extract_df_greedy(y_var_alpha) - greedy_real_val), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")), shape = 0, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = dp_x, y = extract_df_dp(y_var_alpha) - dp_real_val), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")), shape = 1, size = point_size)
        cur <- cur + geom_point(data = data.frame(x = fr_x, y = extract_df_fr(y_var_alpha) - fr_real_val), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")), shape = 2, size = point_size)
    }

    if (plot_runtime) {
        cur <- cur + geom_point(data = data.frame(x = greedy_x, y = extract_df_greedy(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
        cur <- cur + geom_point(data = data.frame(x = dp_x, y = extract_df_dp(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
        cur <- cur + geom_point(data = data.frame(x = fr_x, y = extract_df_fr(paste(c, "_MFC_Total_Runtime", sep = "")) / 1000.0), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
    }

    if (plot_distc) {
        cur <- cur + geom_point(data = data.frame(x = greedy_x, y = extract_df_greedy(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("Greedy", alpha_lab, sep = "")))
        cur <- cur + geom_point(data = data.frame(x = dp_x, y = extract_df_dp(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("DP", alpha_lab, sep = "")))
        cur <- cur + geom_point(data = data.frame(x = fr_x, y = extract_df_fr(paste(c, "_Dist_Calls_mu", sep = ""))), aes(x = x, y = y, color = paste("FR", alpha_lab, sep = "")))
    }

    # Rescale y axis using a log scale
    if (y_log) {
        # Extra text formatting functions because ggplot is interesting...
        parse_safe <- function(text) {
            out <- vector("expression", length(text))
            for (i in seq_along(text)) {
                expr <- parse(text = text[[i]])
                out[[i]] <- if (length(expr) == 0) NA else expr[[1]]
            }
            out
        }

        format_log2 <- function(x, base = 10, signed = NULL, ...) {
            if (length(x) == 0) {
                return(character())
            }
            prefix <- rep("", length(x))
            finites <- x[is.finite(x)]
            signed <- signed %||% any(finites <= 0)
            exponent <- sprintf("'%.1f'", log(x, base = base))
            text <- paste0(prefix, base, "^", exponent)
            text[is.na(x)] <- NA
            text
        }


        cur <- cur + scale_y_continuous(trans = "log10", labels = function(x) {
            text <- format_log2(x, base = 10, signed = FALSE, digits = 1)
            ret <- parse_safe(text)
            ret[is.na(x)] <- NA
            ret
        })

        if (args[9] != "max") {
            mn <- as.numeric(args[9])
            mx <- as.numeric(args[10])
            cur <- cur + coord_cartesian(xlim = c(mn, mx))
        }
    } else {
        # cur <- cur + scale_y_continuous(labels = number_format(accuracy = 1, signed = FALSE))
    }

    # Remove x and y axis labels
    if (args[6] == "no") {
        cur <- cur + theme(axis.title.x = element_blank())
    }

    if (args[7] == "no") {
        cur <- cur + theme(axis.title.y = element_blank())
    }

    print(cur)
}

# Loop over all c and generates plots
for (c in c_counts) {
    plot_alpha(title_prefix, c)
}
