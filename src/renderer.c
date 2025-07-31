//Visualization: export .ppm
#include <stdio.h>
#include "renderer.h"
#include "map_visuals.h"  

void render_to_ppm(const char *filename, int **map, int width, int height) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Could not create .ppm file");
        return;
    }

    //Write PPM header (format P3 = text RGB)
    fprintf(f, "P3\n%d %d\n255\n", width, height);

    //Write RGB values per cell
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int r, g, b;
            get_cell_color(map[i][j], &r, &g, &b);
            fprintf(f, "%d %d %d ", r, g, b);
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Map rendered to %s\n", filename);
}
