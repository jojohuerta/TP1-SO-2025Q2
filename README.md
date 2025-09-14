# TP1-SO-2025Q2
Trabajo Práctico de Sistemas Operativos. 

# ChompChamps

ChompChamps es un juego multijugador del género snake. El tablero de juego es una grilla rectangular donde cada celda contiene recompensas. Al inicio, los jugadores son ubicados en diferentes posiciones y a medida que se desplazan, obtienen las recompensas de las celdas que visitan.

---

## Requisitos

- **Docker instalado.**
- **Imagen Docker** provista por la cátedra:  
  `agodio/itba-so-multi-platform:3.0`
- Clon de este repositorio.
- PVS con una licencia (Opcional).
  
---

## Inicialización del Entorno

Para comenzar a trabajar dentro del entorno de desarrollo controlado por la cátedra, ejecutar:

```bash
docker run --rm -v ${PWD}:/root --security-opt seccomp:unconfined -it agodio/itba-so-multi-platform:3.0
```

Luego, moverse al directorio `root`. Puede hacerse con:

```bash
cd root
```

---

## Compilación

El proyecto cuenta con un `Makefile` que facilita la compilación de los distintos binarios del sistema:

```bash
make all
```

Este comando genera tres ejecutables:
- `ChompChamps` → Binario principal del juego.
- `player` → Ejecutable que representa a un jugador.
- `view` → Binario opcional para mostrar visualmente el juego.

Para más información acerca de los comandos disponibles con el `Makefile`, ver sección `Funcionalidades del Makefile`.

---

## Ejecución

El binario principal `ChompChamps` acepta múltiples parámetros para configurar la partida:

```bash
./ChompChamps -p player [player2 ... playerN] [-v view] [-w WIDTH] [-h HEIGHT] [-d DELAY] [-t TIMEOUT] [-s SEED]
```

### Parámetros

●​[-p]: Ruta/s de los binarios de los jugadores. Mínimo: 1, Máximo: 9

●​[-w]: Ancho del tablero. Default y mínimo: 10

●​[-h]: Alto del tablero. Default y mínimo: 10

●​[-d]: milisegundos que espera el máster cada vez que se imprime el estado. Default: 200

●​[-t]: Timeout en segundos para recibir solicitudes de movimientos válidos. Default: 10

●​[-s]: Semilla utilizada para la generación del tablero. Default: time(NULL)

●​[-v]: Ruta del binario de la vista. Default: Sin vista.

### Todos los parametros son opcionales excepto el parámetro -p. 

---

## Funcionalidades del Makefile

El `Makefile` permite realizar múltiples tareas relacionadas con el desarrollo y análisis del proyecto:

### Comandos disponibles

- **Compilar todos los binarios:**
  ```bash
  make all
  ```

- **Compilar individualmente:**
  - Juego:
    ```bash
    make ChompChamps
    ```
  - Vista:
    ```bash
    make view
    ```
  - Jugador:
    ```bash
    make player
    ```

- **Limpiar binarios generados:**
  ```bash
  make clean
  ```

- **Limpiar archivos temporales, analizar con PVS-Studio y generar un reporte**
  ```bash
  make pvs
  ```

  Para ejecutar este comando es necesario tener instalado PVS-Studio y tener una licencia.
  
  El reporte HTML se encuentra en `informe_completo.html`.
---
