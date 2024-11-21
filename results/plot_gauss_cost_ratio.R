library("ggplot2")
library(reshape2)
library(latex2exp)

args <- commandArgs(trailingOnly = TRUE)

df <- read.csv(args[1])

pdf(args[2])

df$C16_MFC_Cost_Ratio <- df$C16_MFC_Cost_mu / df$MST_Cost_mu
df$C32_MFC_Cost_Ratio <- df$C32_MFC_Cost_mu / df$MST_Cost_mu
df$C64_MFC_Cost_Ratio <- df$C64_MFC_Cost_mu / df$MST_Cost_mu
df$C128_MFC_Cost_Ratio <- df$C128_MFC_Cost_mu / df$MST_Cost_mu
df$C256_MFC_Cost_Ratio <- df$C256_MFC_Cost_mu / df$MST_Cost_mu

df$C16_MFC_Test_Ratio <- (1 + 2 * df$C16_Gamma_mu) / df$C16_MFC_Cost_Ratio
df$C32_MFC_Test_Ratio <- (1 + 2 * df$C32_Gamma_mu) / df$C32_MFC_Cost_Ratio
df$C64_MFC_Test_Ratio <- (1 + 2 * df$C64_Gamma_mu) / df$C64_MFC_Cost_Ratio
df$C128_MFC_Test_Ratio <- (1 + 2 * df$C128_Gamma_mu) / df$C128_MFC_Cost_Ratio
df$C256_MFC_Test_Ratio <- (1 + 2 * df$C256_Gamma_mu) / df$C256_MFC_Cost_Ratio

font_size <- 30
line_width <- 2
point_size <- 2

draw_plot <- function(df, names, title, x_axis, y_axis, legend_title, legend_names, legend_pos = c(0.84, 0.83), legend = FALSE, vert_lines = TRUE) {
    df_melt <- melt(df, id.vars = "GaussCount", measure.vars = names)

    graph <- ggplot(df_melt, aes(x = GaussCount, y = value, group = variable)) +
        geom_point(aes(color = variable, shape = variable), show.legend = legend, size = point_size) +
        labs(x = x_axis, y = y_axis) +
        ggtitle(title) +
        labs(shape = legend_title, color = legend_title) +
        scale_colour_discrete(labels = legend_names) +
        scale_shape_discrete(labels = legend_names) +
        theme_bw() +
        theme(
            axis.title = element_text(size = font_size),
            axis.text.x = element_text(size = font_size),
            axis.text.y = element_text(size = font_size + 3),
            text = element_text(family = "Times"),
            plot.title = element_text(hjust = 0.5),
            legend.box.background = element_rect(colour = "black", linewidth = line_width),
            legend.position = "inside",
            legend.position.inside = legend_pos,
            legend.text = element_text(size = font_size - 5),
            panel.grid.major = element_blank(),
            panel.grid.minor = element_blank(),
            axis.ticks = element_line(linewidth = line_width),
            axis.ticks.length = unit(.25, "cm"),
            panel.border = element_rect(colour = "black", fill = NA, linewidth = line_width),
            legend.title = element_blank(),
        ) +
        scale_x_continuous(breaks = c(32, 64, 128, 256)) +
        guides(color = guide_legend(override.aes = list(size = 5)))

    if (vert_lines) {
        graph <- graph +
            geom_vline(xintercept = 16, linetype = "dashed") +
            geom_vline(xintercept = 32, linetype = "dashed") +
            geom_vline(xintercept = 64, linetype = "dashed") +
            geom_vline(xintercept = 128, linetype = "dashed") +
            geom_vline(xintercept = 256, linetype = "dashed")
    }

    print(graph)
}

draw_plot(
    df,
    c("C16_MFC_Test_Ratio", "C32_MFC_Test_Ratio", "C64_MFC_Test_Ratio", "C128_MFC_Test_Ratio", "C256_MFC_Test_Ratio"),
    "",
    TeX("Number of Gaussians $(g)$"),
    TeX("$(2 \\gamma + 1) / ($Cost Ratio$)"),
    "Clusters",
    c("t = 16", "t = 32", "t = 64", "t = 128", "t = 256"),
    legend = TRUE,
)

dev.off()
