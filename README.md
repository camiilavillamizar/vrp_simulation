# Parallel Simulation + Frontend Viewer (MPI + JS)

This project is a parallelized simulation using **MPI (OpenMPI)** in C, with a simple **JavaScript frontend** to replay the results tick-by-tick.

---

## Step-by-Step Instructions

### 1️. Build the Docker image

This creates a container with everything needed (OpenMPI, JSON-C, Python3, etc).

```bash
docker build -t mympi .
```

### 2. Run the container

#### macOs/Linux

```bash
docker run -it --rm -v "$PWD":/workspace -p 8080:8080 mympi /bin/bash
```
#### Windows (CMD)
```bash
docker run -it --rm -v %cd%:/workspace -p 8080:8080 mympi /bin/bash
```

#### Windows (PowerShell)
```bash
docker run -it --rm -v ${PWD}:/workspace -p 8080:8080 mympi /bin/bash
```

### 3. Run the simulation + start the frontend
```bash
make clean
make run

```

### 3. View the result in your browser
Open: http://localhost:8080/index.html


## Notes
To change simulation values is it possible to modify the file include/game_rules.h, lines 42 to 55
