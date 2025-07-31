//ticks engine and principal cicle
#include "map.h"
#include "renderer.h"
#include "villager.h"

int main() {
    //Generate random map and villagers
    //generate_random_map();
    load_map_from_file("output/map/initial_map.txt"); //We can use this when we want to load the map

    place_villagers_on_map();

    // Render the current map to PPM image
    render_to_ppm("output/map/initial_map.ppm", game_map, MAP_WIDTH, MAP_HEIGHT);
    
    

    print_map_viewport(50, 25, 100, 50);





    // Free allocated memory
    free_map();

    return 0;
}
