library("ggplot2")
library(reshape2)
library(latex2exp)

args <- commandArgs(trailingOnly = TRUE)

df <- read.csv(args[1])

pdf(args[2])

font_size <- 30
line_width <- 2
point_size <- 2
legend <- FALSE
legend_pos <- c(0.14, 0.82)

graph <- ggplot(df, aes(x = x, y = y, group = g)) +
    geom_point(aes(color = g, shape = g), show.legend = legend, size = 3) +
    labs(x = "x", y = "y") +
    ggtitle("2d Gauss Test") +
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
    guides(color = guide_legend(override.aes = list(size = 5))) +
    scale_shape_identity()

print(graph)

dev.off()
