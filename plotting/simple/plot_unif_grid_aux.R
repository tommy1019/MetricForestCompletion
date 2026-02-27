library("ggplot2")
library(reshape2)
library(latex2exp)
library(cowplot)

get_plots <- function(filename, axis_label_mask, title) {
  df <- read.csv(filename)

  x_lab <- "n"

  if (axis_label_mask == "x" || axis_label_mask == "xy") {
    x_lab <- ""
  }

  df$C16_MFC_Cost_Ratio <- df$C16_MFC_Cost_mu / df$MST_Cost_mu
  df$C32_MFC_Cost_Ratio <- df$C32_MFC_Cost_mu / df$MST_Cost_mu
  df$C64_MFC_Cost_Ratio <- df$C64_MFC_Cost_mu / df$MST_Cost_mu
  df$C128_MFC_Cost_Ratio <- df$C128_MFC_Cost_mu / df$MST_Cost_mu
  df$C256_MFC_Cost_Ratio <- df$C256_MFC_Cost_mu / df$MST_Cost_mu

  df$C16_MFC_Runtime_Ratio <- df$MST_Runtime_mu / df$C16_MFC_Runtime_mu
  df$C32_MFC_Runtime_Ratio <- df$MST_Runtime_mu / df$C32_MFC_Runtime_mu
  df$C64_MFC_Runtime_Ratio <- df$MST_Runtime_mu / df$C64_MFC_Runtime_mu
  df$C128_MFC_Runtime_Ratio <- df$MST_Runtime_mu / df$C128_MFC_Runtime_mu
  df$C256_MFC_Runtime_Ratio <- df$MST_Runtime_mu / df$C256_MFC_Runtime_mu

  font_size <- 36
  line_width <- 2
  point_size <- 2

  draw_plot <- function(df, names, title, x_axis, y_axis, legend_title, legend_names, legend_pos = c(0.86, 0.82)) {
    df_melt <- melt(df, id.vars = "N_mu", measure.vars = names)

    if (axis_label_mask == "y" || axis_label_mask == "xy") {
      y_axis <- ""
    }

    graph <- ggplot(df_melt, aes(x = N_mu, y = value, group = variable)) +
      geom_point(aes(color = variable, shape = variable), show.legend = FALSE, size = point_size) +
      ggtitle(title) +
      labs(shape = legend_title, color = legend_title) +
      labs(x = x_axis, y = y_axis) +
      scale_colour_discrete(labels = legend_names) +
      scale_shape_discrete(labels = legend_names) +
      scale_x_continuous(breaks = scales::pretty_breaks(n = 3)) +
      scale_y_continuous(breaks = scales::pretty_breaks(n = 6)) +
      theme_bw() +
      theme(
        axis.title = element_text(size = font_size),
        axis.text = element_text(size = font_size),
        axis.text.x = element_text(angle = 0, hjust = 1, size = font_size),
        axis.text.y = element_text(size = font_size + 3),
        text = element_text(family = "Times"),
        plot.title = element_text(size = font_size, hjust = 0.5),
        legend.box.background = element_rect(colour = "black", linewidth = line_width),
        legend.position = "inside",
        legend.position.inside = legend_pos,
        legend.title = element_text(size = font_size),
        legend.text = element_text(size = font_size - 5),
        panel.grid.major = element_blank(),
        panel.grid.minor = element_blank(),
        axis.ticks = element_line(linewidth = line_width),
        axis.ticks.length = unit(.25, "cm"),
        panel.border = element_rect(colour = "black", fill = NA, linewidth = line_width),
        plot.margin = grid::unit(c(0, 1, 0, 1), "mm") # top, right, bottom, left
      ) +
      guides(color = guide_legend(override.aes = list(size = 5)))

    if (x_axis == "") {
      graph <- graph + theme(axis.title.x = element_blank())
    }

    if (y_axis == "") {
      graph <- graph + theme(axis.title.y = element_blank())
    }

    if (title == "") {
      graph <- graph + theme(plot.title = element_blank())
    }

    return(graph)
  }

  p1 <- draw_plot(
    df,
    c("C16_Gamma_mu", "C32_Gamma_mu", "C64_Gamma_mu", "C128_Gamma_mu", "C256_Gamma_mu"),
    "",
    x_lab,
    TeX("$\\gamma$ upper bound"),
    "Clusters",
    c("16", "32", "64", "128", "256")
  )

  p2 <- draw_plot(
    df,
    c("C16_MFC_Cost_Ratio", "C32_MFC_Cost_Ratio", "C64_MFC_Cost_Ratio", "C128_MFC_Cost_Ratio", "C256_MFC_Cost_Ratio"),
    TeX(title),
    x_lab,
    "Cost ratio",
    "Clusters",
    c("16", "32", "64", "128", "256")
  )

  p3 <- draw_plot(
    df,
    c("C16_MFC_Runtime_Ratio", "C32_MFC_Runtime_Ratio", "C64_MFC_Runtime_Ratio", "C128_MFC_Runtime_Ratio", "C256_MFC_Runtime_Ratio"),
    "",
    "n",
    "Runtime ratio",
    "Clusters",
    c("16", "32", "64", "128", "256"),
    legend_pos = c(0.14, 0.82)
  )

  return(list(p1, p2, p3))
}

args <- commandArgs(trailingOnly = TRUE)

pdf(args[1], width = 24, height = 30)

c002 <- get_plots("synth/uniform_2.txt", "x", "Uniform Random Points d=2")
c004 <- get_plots("synth/uniform_4.txt", "x", "Uniform Random Points d=4")
c008 <- get_plots("synth/uniform_8.txt", "x", "Uniform Random Points d=8")
c016 <- get_plots("synth/uniform_16.txt", "x", "Uniform Random Points d=16")
c032 <- get_plots("synth/uniform_32.txt", "x", "Uniform Random Points d=32")
c064 <- get_plots("synth/uniform_64.txt", "x", "Uniform Random Points d=64")
c128 <- get_plots("synth/uniform_128.txt", "x", "Uniform Random Points d=128")
c256 <- get_plots("synth/uniform_256.txt", "x", "Uniform Random Points d=256")

plot_grid(
  c004[[1]], c004[[2]], c004[[3]],
  c008[[1]], c008[[2]], c008[[3]],
  c016[[1]], c016[[2]], c016[[3]],
  c032[[1]], c032[[2]], c032[[3]],
  c064[[1]], c064[[2]], c064[[3]],
  # c128[[1]], c128[[2]], c128[[3]],
  c256[[1]], c256[[2]], c256[[3]],
  align = "hv",
  ncol = 3
)

dev.off()
