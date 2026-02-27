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

  font_size <- 40
  line_width <- 2
  point_size <- 2

  draw_plot <- function(df, names, title, x_axis, y_axis) {
    df_melt <- melt(df, id.vars = "N_mu", measure.vars = names)

    y_marg <- 0

    if (axis_label_mask == "y" || axis_label_mask == "xy") {
      y_axis <- ""
      y_marg <- -400
    }

    graph <- ggplot(df_melt, aes(x = N_mu, y = value, group = variable)) +
      geom_point(aes(color = variable, shape = variable), show.legend = FALSE, size = point_size) +
      ggtitle(title) +
      labs(x = x_axis, y = y_axis) +
      scale_x_continuous(breaks = scales::pretty_breaks(n = 3)) +
      scale_y_continuous(breaks = scales::pretty_breaks(n = 6)) +
      theme_bw() +
      theme(
        axis.title.x = element_text(size = font_size, margin = margin(0, 0, 0, 0)),
        axis.title.y = element_text(size = font_size, margin = margin(0, 0, 0, y_marg)),
        axis.text.x = element_text(angle = 0, hjust = 1, size = font_size, margin = margin(0, 0, 0, 0)),
        axis.text.y = element_text(size = font_size, margin = margin(0, 0, 0, 0)),
        text = element_text(family = "Times", margin = margin(0, 0, 0, 0)),
        plot.title = element_text(size = font_size, hjust = 0.5, margin = margin(0, 60, 0, 0)),
        panel.grid.major = element_blank(),
        panel.grid.minor = element_blank(),
        axis.ticks = element_line(linewidth = line_width),
        axis.ticks.length = unit(.35, "cm"),
        panel.border = element_rect(colour = "black", fill = NA, linewidth = line_width),
        strip.clip = "off",
        # plot.background = element_rect(fill = "yellow"),
        plot.margin = grid::unit(c(0, 1, 0, 1), "mm") # top, right, bottom, left
      )

    return(graph)
  }

  p1 <- draw_plot(
    df,
    c("C16_Gamma_mu", "C32_Gamma_mu", "C64_Gamma_mu", "C128_Gamma_mu", "C256_Gamma_mu"),
    TeX(title),
    x_lab,
    TeX("$\\gamma$ upper bound")
  )

  p2 <- draw_plot(
    df,
    c("C16_MFC_Cost_Ratio", "C32_MFC_Cost_Ratio", "C64_MFC_Cost_Ratio", "C128_MFC_Cost_Ratio", "C256_MFC_Cost_Ratio"),
    "",
    x_lab,
    "Cost ratio"
  )

  p3 <- draw_plot(
    df,
    c("C16_MFC_Runtime_Ratio", "C32_MFC_Runtime_Ratio", "C64_MFC_Runtime_Ratio", "C128_MFC_Runtime_Ratio", "C256_MFC_Runtime_Ratio"),
    "",
    "n",
    "Runtime ratio"
  )

  return(list(p1, p2, p3))
}

args <- commandArgs(trailingOnly = TRUE)

pdf(args[1], width = 16, height = 20)

c1 <- get_plots("synth/uniform_8.txt", "x", "Uniform Random Points d=8")
c2 <- get_plots("synth/uniform_256.txt", "xy", "Uniform Random Points d=256")

plot_grid(
  c1[[1]], c2[[1]],
  c1[[2]], c2[[2]],
  c1[[3]], c2[[3]],
  align = "hv",
  ncol = 2
)

dev.off()
