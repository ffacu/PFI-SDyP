library(ggplot2)
library(tidyr)
library(dplyr)

df <- read_csv("/home/franco/metricas.csv")


# --------------------------------
# 800, 3 - 3
# --------------------------------

df_800_3_3 <- df %>%
  filter(
    Imagen == "img_800.jpg"
    & Filtro == 3
    & Niveles == 3
  )

# Transformar a formato largo
df_largo <- df_800_3_3 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


# Grafico de líneas
ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 80, by = 5)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 800x800 | Filtro: 3 | Niveles: 3",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())

# --------------------------------
# 800, 5 - 9
# --------------------------------

df_800_5_9 <- df %>%
  filter(
    Imagen == "img_800.jpg"
    & Filtro == 5
    & Niveles == 9
  )

df_largo <- df_800_5_9 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 80, by = 5)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 800x800 | Filtro: 5 | Niveles: 9",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------------
# 2000, 3 - 3
# --------------------------------

df_2000_3_3 <- df %>%
  filter(
    Imagen == "img_2000.jpg"
    & Filtro == 3
    & Niveles == 3
  )

df_largo <- df_2000_3_3 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 800, by = 50)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 2000x2000 | Filtro: 3 | Niveles: 3",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------------
# 2000, 5 - 9
# --------------------------------

df_2000_5_9 <- df %>%
  filter(
    Imagen == "img_2000.jpg"
    & Filtro == 5
    & Niveles == 9
  )

df_largo <- df_2000_5_9 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 800, by = 50)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 2000x2000 | Filtro: 5 | Niveles: 9",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------------
# 5000, 3 - 3
# --------------------------------

df_5000_3_3 <- df %>%
  filter(
    Imagen == "img_5000.jpg"
    & Filtro == 3
    & Niveles == 3
  )

df_largo <- df_5000_3_3 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 2000, by = 100)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 5000x5000 | Filtro: 3 | Niveles: 3",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------------
# 5000, 5 - 9
# --------------------------------

df_5000_5_9 <- df %>%
  filter(
    Imagen == "img_5000.jpg"
    & Filtro == 5
    & Niveles == 9
  )

df_largo <- df_5000_5_9 %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))


ggplot(df_largo, aes(x = Iteracion_num, y = Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 2900, by = 100)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación de Tiempos de Ejecución",
       subtitle = "Imagen: 5000x5000 | Filtro: 5 | Niveles: 9",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------------------------------------------------------
# Dataframe que contiene para cada programa, el mejor tiempo obtenido en 
# cada tipo de ejecucion. Es decir, la grafica tendra los 4 programas,
# cada valor de x sera una combinacion de esta manera:
# 800-3-3, 800-5-9, 2000-3-3, 2000-5-9, 5000-3-3, 5000-5-9
# para asi poder ver como evolucionan los tiempos a medida que cambia 
# el contexto de ejecucion. Sabremos asi cual es el mas estable, cual
# crece mas rapido y si hay alguna relacion alli. Se considerara el menor
# valor obtenido en cada caso.
# --------------------------------------------------------------------------

df_largo <- df %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(
    Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)),
    # Crear columna de contexto
    Contexto = paste(Imagen, Filtro, Niveles, sep = "-")
  )

# Crear dataframe resumen con el minimo tiempo por contexto y programa
df_mejores <- df_largo %>%
  group_by(Imagen, Filtro, Niveles, Programa, Contexto) %>%
  summarise(
    Mejor_Tiempo = min(Tiempo, na.rm = TRUE),
    Iteracion_mejor = Iteracion_num[which.min(Tiempo)],
    .groups = "drop"
  )


df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_800.jpg" &
                           Filtro == 3 &
                           Niveles == 3,
                           "img_800-3-3",
                           Contexto))

df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_800.jpg" &
                             Filtro == 5 &
                             Niveles == 9,
                           "img_800-5-9",
                           Contexto))

df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_2000.jpg" &
                             Filtro == 3 &
                             Niveles == 3,
                           "img_2000-3-3",
                           Contexto))

df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_2000.jpg" &
                             Filtro == 5 &
                             Niveles == 9,
                           "img_2000-5-9",
                           Contexto))

df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_5000.jpg" &
                             Filtro == 3 &
                             Niveles == 3,
                           "img_5000-3-3",
                           Contexto))

df_mejores <- df_mejores %>%
  mutate(Contexto = ifelse(Imagen == "img_5000.jpg" &
                             Filtro == 5 &
                             Niveles == 9,
                           "img_5000-5-9",
                           Contexto))

orden_contextos <- c("img_800-3-3", "img_800-5-9", 
                     "img_2000-3-3", "img_2000-5-9",
                     "img_5000-3-3", "img_5000-5-9")

df_mejores$Contexto <- factor(df_mejores$Contexto, levels = orden_contextos)


ggplot(df_mejores, aes(x = Contexto, y = Mejor_Tiempo, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.0) +
  geom_point(size = 2.5) +
  scale_y_continuous(breaks = seq(0, 2900, by = 100)) + 
  scale_color_manual(values = c("Secuencial" = "red", 
                                "OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Comparación General de Tiempos",
       subtitle = "Mínimo tiempo obtenido en cada ejecución",
       x = "Iteración",
       y = "Tiempo (ms)",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_blank())


# --------------------------
# SpeedUp y Eficiencia
# --------------------------

df_speedup <- df_mejores %>%
  # Obtener el tiempo secuencial para cada contexto
  group_by(Contexto) %>%
  mutate(Tiempo_Secuencial = Mejor_Tiempo[Programa == "Secuencial"]) %>%
  ungroup() %>%
  # Filtrar solo los programas que no son secuencial
  filter(Programa != "Secuencial") %>%
  # Calcular speedup = Tiempo_Secuencial / Tiempo_del_programa
  mutate(Speedup = Tiempo_Secuencial / Mejor_Tiempo)


ggplot(df_speedup, aes(x = Contexto, y = Speedup, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.2) +
  geom_point(size = 3) +
  scale_y_continuous(breaks = seq(0, 12, by = 1)) + 
  geom_hline(yintercept = 1, linetype = "dashed", color = "gray50", linewidth = 0.8) +  # Línea de referencia (igual que secuencial)
  geom_text(aes(label = round(Speedup, 2)), 
            vjust = -0.8, 
            size = 3.5,
            show.legend = FALSE) +
  scale_color_manual(values = c("OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Speedup",
       x = "Configuración",
       y = "Speedup",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        axis.text.x = element_text(angle = 45, hjust = 1),
        panel.grid.minor = element_line(linetype = "dotted"))

# --- Eficiencia ---

df_eficiencia <- df_speedup %>%
  # Calcular eficiencia segun el programa
  mutate(Eficiencia = case_when(
    Programa == "OpenMP" ~ Speedup / 32,
    Programa == "MPI" ~ Speedup / 128,
    Programa == "Hibrido" ~ Speedup / 128,
    TRUE ~ NA_real_
  ))


ggplot(df_eficiencia, aes(x = Contexto, y = Eficiencia, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.2) +
  geom_point(size = 3) +
  scale_y_continuous(breaks = seq(0, 0.5, by = 0.05)) + 
  geom_text(aes(label = round(Eficiencia, 3)), 
            vjust = -0.8, 
            size = 3.5,
            show.legend = FALSE) +
  scale_color_manual(values = c("OpenMP" = "blue", 
                                "MPI" = "green", 
                                "Hibrido" = "purple")) +
  labs(title = "Eficiencia",
       subtitle = "OpenMP: 32 núcleos | MPI/Híbrido: 128 núcleos",
       x = "Configuración",
       y = "Eficiencia",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        axis.text.x = element_text(angle = 45, hjust = 1),
        panel.grid.minor = element_line(linetype = "dotted"))


# ===================================================================
# ANÁLISIS DE ESCALABILIDAD DE HILOS
# Imagen: 5000x5000 | Filtro: 5x5 | Niveles: 9
# Se varía la cantidad de hilos: 1, 2, 4, 8, 16, 32
# ===================================================================

df_threads <- read_csv("/home/franco/metricas_threads.csv")

# Transformar a formato largo
df_threads_largo <- df_threads %>%
  pivot_longer(cols = starts_with("Iter"), 
               names_to = "Iteracion", 
               values_to = "Tiempo") %>%
  mutate(Iteracion_num = as.numeric(gsub("Iter", "", Iteracion)))

# Obtener el mejor (mínimo) tiempo por programa y cantidad de hilos
df_threads_mejor <- df_threads_largo %>%
  group_by(Programa, Threads) %>%
  summarise(
    Mejor_Tiempo = min(Tiempo, na.rm = TRUE),
    .groups = "drop"
  )

# Calcular Speedup relativo: S(p) = T(1 hilo) / T(p hilos)
# Cada programa usa su propio T(1) como referencia
df_threads_speedup <- df_threads_mejor %>%
  group_by(Programa) %>%
  mutate(
    Tiempo_Base = Mejor_Tiempo[Threads == 1],
    Speedup = Tiempo_Base / Mejor_Tiempo
  ) %>%
  ungroup()

# Línea de speedup ideal (S = p)
df_ideal <- data.frame(Threads = c(1, 32), Speedup = c(1, 32))


# --- Gráfico de Speedup vs Hilos ---
ggplot(df_threads_speedup, aes(x = Threads, y = Speedup, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.2) +
  geom_point(size = 3) +
  geom_line(data = df_ideal, aes(x = Threads, y = Speedup),
            linetype = "dashed", color = "gray50", linewidth = 0.8,
            inherit.aes = FALSE) +
  annotate("text", x = 28, y = 30, label = "Speedup ideal (S = p)",
           color = "gray40", size = 3.5, fontface = "italic") +
  geom_text(aes(label = round(Speedup, 2)), 
            vjust = -0.8, 
            size = 3.5,
            show.legend = FALSE) +
  scale_x_continuous(breaks = c(1, 2, 4, 8, 16, 32)) +
  scale_y_continuous(breaks = seq(0, 32, by = 2)) +
  scale_color_manual(values = c("OpenMP" = "blue", "Hibrido" = "purple")) +
  labs(title = "Speedup según cantidad de hilos",
       subtitle = "Imagen: 5000x5000 | Filtro: 5x5 | 9 niveles | Referencia: T(1 hilo)",
       x = "Cantidad de hilos",
       y = "Speedup",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_line(linetype = "dotted"))


# --- Gráfico de Eficiencia vs Hilos ---
# E(p) = S(p) / p, donde p es la cantidad de hilos
df_threads_eficiencia <- df_threads_speedup %>%
  mutate(Eficiencia = Speedup / Threads)

ggplot(df_threads_eficiencia, aes(x = Threads, y = Eficiencia, color = Programa, group = Programa)) +
  geom_line(linewidth = 1.2) +
  geom_point(size = 3) +
  geom_hline(yintercept = 1, linetype = "dashed", color = "gray50", linewidth = 0.8) +
  annotate("text", x = 28, y = 1.03, label = "Eficiencia ideal (E = 1)",
           color = "gray40", size = 3.5, fontface = "italic") +
  geom_text(aes(label = round(Eficiencia, 3)), 
            vjust = -0.8, 
            size = 3.5,
            show.legend = FALSE) +
  scale_x_continuous(breaks = c(1, 2, 4, 8, 16, 32)) +
  scale_y_continuous(breaks = seq(0, 1.1, by = 0.1), limits = c(0, 1.15)) +
  scale_color_manual(values = c("OpenMP" = "blue", "Hibrido" = "purple")) +
  labs(title = "Eficiencia según cantidad de hilos",
       subtitle = "Imagen: 5000x5000 | Filtro: 5x5 | 9 niveles | E = S(p) / p",
       x = "Cantidad de hilos",
       y = "Eficiencia",
       color = "Programa") +
  theme_minimal() +
  theme(legend.position = "bottom",
        panel.grid.minor = element_line(linetype = "dotted"))
