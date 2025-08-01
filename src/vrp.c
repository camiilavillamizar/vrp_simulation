#include "vrp.h"
#include <limits.h>
#include <stdlib.h>
#include <math.h>

// ===========================================
// Queue Implementation for BFS Pathfinding
// ===========================================

// Queue node structure
typedef struct QueueNode
{
  int x, y;               // Position stored in the node
  struct QueueNode *next; // Pointer to next node
} QueueNode;

// Queue structure
typedef struct
{
  QueueNode *front; // Front pointer
  QueueNode *rear;  // Rear pointer
} Queue;

// Create an empty queue
Queue *create_queue()
{
  Queue *q = (Queue *)malloc(sizeof(Queue));
  q->front = q->rear = NULL;
  return q;
}

// Add position to rear of queue
void enqueue(Queue *q, int x, int y)
{
  QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
  node->x = x;
  node->y = y;
  node->next = NULL;

  if (q->rear)
  {
    q->rear->next = node;
  }
  else
  {
    q->front = node;
  }
  q->rear = node;
}

// Remove and return position from front of queue
void dequeue(Queue *q, int *x, int *y)
{
  if (!q->front)
  {
    *x = *y = -1; // Invalid position
    return;
  }

  QueueNode *temp = q->front;
  *x = temp->x;
  *y = temp->y;

  q->front = q->front->next;
  if (!q->front)
    q->rear = NULL;

  free(temp);
}

// Check if queue is empty
int is_queue_empty(Queue *q)
{
  return q->front == NULL;
}

// Free all memory used by queue
void free_queue(Queue *q)
{
  int x, y;
  while (!is_queue_empty(q))
  {
    dequeue(q, &x, &y);
  }
  free(q);
}

// ===========================================
// Pathfinding and Reachability
// ===========================================

/**
 * Check if a target position is reachable from start position using BFS
 *
 * @param map The game map
 * @param start_x, start_y Starting position
 * @param target_x, target_y Target position
 * @return 1 if reachable, 0 otherwise
 */
int is_reachable(int map[MAP_HEIGHT][MAP_WIDTH], int start_x, int start_y, int target_x, int target_y)
{
  // If start and target are the same, always reachable
  if (start_x == target_x && start_y == target_y)
    return 1;

  // Create visited matrix (initialize to 0)
  int visited[MAP_HEIGHT][MAP_WIDTH] = {0};

  // Create and initialize queue
  Queue *q = create_queue();
  enqueue(q, start_x, start_y);
  visited[start_y][start_x] = 1;

  // Direction vectors: right, left, down, up
  int dx[] = {1, -1, 0, 0};
  int dy[] = {0, 0, 1, -1};

  while (!is_queue_empty(q))
  {
    int cur_x, cur_y;
    dequeue(q, &cur_x, &cur_y);

    // Explore all 4 directions
    for (int d = 0; d < 4; d++)
    {
      int nx = cur_x + dx[d];
      int ny = cur_y + dy[d];

      // Skip if out of bounds
      if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT)
        continue;

      // Skip if already visited
      if (visited[ny][nx])
        continue;

      // Skip if cell is impassable (water, obstacles, etc.)
      if (map[ny][nx] > CELL_TITAN_GATE)
        continue;

      // Check if we reached the target
      if (nx == target_x && ny == target_y)
      {
        free_queue(q);
        return 1;
      }

      // Mark as visited and enqueue
      visited[ny][nx] = 1;
      enqueue(q, nx, ny);
    }
  }

  // Target not reachable
  free_queue(q);
  return 0;
}

// ===========================================
// Strategy Implementations
// ===========================================

/**
 * Greedy strategy: Assigns the closest available resource to each villager
 */
StrategyResult assign_task_greedy(Map *map, Villager *villagers, int num_villagers)
{
  StrategyResult result = {0, 0};

  for (int i = 0; i < num_villagers; i++)
  {
    Task best_task = {{-1, -1}, CELL_EMPTY};
    int min_dist = INT_MAX;

    // Scan entire map for resources
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
      for (int x = 0; x < MAP_WIDTH; x++)
      {
        int cell = map->cells[y][x];

        // Only consider resource cells
        if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD)
          continue;

        // Skip unreachable resources
        if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
          continue;

        // Calculate Manhattan distance
        int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

        // Update best task if this resource is closer
        if (dist < min_dist)
        {
          min_dist = dist;
          best_task.target.x = x;
          best_task.target.y = y;
          best_task.type = cell;
        }
      }
    }

    // If we found a valid task
    if (best_task.type != CELL_EMPTY)
    {
      // Calculate movement distance
      int dist = abs(villagers[i].x - best_task.target.x) +
                 abs(villagers[i].y - best_task.target.y);

      // Calculate gathering time based on resource type
      int gather_ticks = 0;
      switch (best_task.type)
      {
      case CELL_WOOD:
        gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY;
        break;
      case CELL_FOOD:
        gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY;
        break;
      case CELL_GOLD:
        gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY;
        break;
      }

      // Update total ticks (movement + gathering)
      result.total_ticks += dist + gather_ticks;

      // Update total collected resources
      result.total_collected += VILLAGER_CAPACITY;
    }
  }

  return result;
}

/**
 * Max Profit strategy: Assigns resources with the highest efficiency
 */
StrategyResult assign_task_max_profit(Map *map, Villager *villagers, int num_villagers)
{
  StrategyResult result = {0, 0};

  for (int i = 0; i < num_villagers; i++)
  {
    Task best_task = {{-1, -1}, CELL_EMPTY};
    float max_score = -INFINITY;

    // Scan entire map for resources
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
      for (int x = 0; x < MAP_WIDTH; x++)
      {
        int cell = map->cells[y][x];
        int gather_ticks_per_unit = 0;

        // Determine gathering time per unit
        switch (cell)
        {
        case CELL_WOOD:
          gather_ticks_per_unit = GATHER_TICK_PER_WOOD;
          break;
        case CELL_FOOD:
          gather_ticks_per_unit = GATHER_TICK_PER_FOOD;
          break;
        case CELL_GOLD:
          gather_ticks_per_unit = GATHER_TICK_PER_GOLD;
          break;
        default:
          continue; // Skip non-resource cells
        }

        // Skip unreachable resources
        if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
          continue;

        // Calculate Manhattan distance
        int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

        // Calculate total time (movement + gathering)
        int total_ticks = dist + (gather_ticks_per_unit * VILLAGER_CAPACITY);

        // Calculate efficiency score (resources per tick)
        float score = (float)VILLAGER_CAPACITY / total_ticks;

        // Update best task if this resource has higher efficiency
        if (score > max_score)
        {
          max_score = score;
          best_task.target.x = x;
          best_task.target.y = y;
          best_task.type = cell;
        }
      }
    }

    // If we found a valid task
    if (best_task.type != CELL_EMPTY)
    {
      // Calculate movement distance
      int dist = abs(villagers[i].x - best_task.target.x) +
                 abs(villagers[i].y - best_task.target.y);

      // Calculate gathering time based on resource type
      int gather_ticks = 0;
      switch (best_task.type)
      {
      case CELL_WOOD:
        gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY;
        break;
      case CELL_FOOD:
        gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY;
        break;
      case CELL_GOLD:
        gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY;
        break;
      }

      // Update total ticks (movement + gathering)
      result.total_ticks += dist + gather_ticks;

      // Update total collected resources
      result.total_collected += VILLAGER_CAPACITY;
    }
  }

  return result;
}

/**
 * Stage-Based strategy: Assigns resources based on villager's age
 */
StrategyResult assign_task_stage_based(Map *map, Villager *villagers, int num_villagers)
{
  StrategyResult result = {0, 0};

  for (int i = 0; i < num_villagers; i++)
  {
    int preferred_resource = CELL_WOOD;

    // Determine preferred resource based on age
    // switch (villagers[i].age)
    // {
    // case AGE_CLASSICAL:
    //   preferred_resource = CELL_WOOD;
    //   break;
    // case AGE_HEROIC:
    //   preferred_resource = CELL_GOLD;
    //   break;
    // default:
    //   preferred_resource = CELL_FOOD; // Default for other ages
    // }

    Task best_task = {{-1, -1}, CELL_EMPTY};
    int min_dist = INT_MAX;

    // First pass: look for preferred resource
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
      for (int x = 0; x < MAP_WIDTH; x++)
      {
        if (map->cells[y][x] != preferred_resource)
          continue;

        // Skip unreachable resources
        if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
          continue;

        // Calculate Manhattan distance
        int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

        // Update best task if this resource is closer
        if (dist < min_dist)
        {
          min_dist = dist;
          best_task.target.x = x;
          best_task.target.y = y;
          best_task.type = preferred_resource;
        }
      }
    }

    // If preferred resource not found, search for any resource
    if (best_task.type == CELL_EMPTY)
    {
      for (int y = 0; y < MAP_HEIGHT; y++)
      {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
          int cell = map->cells[y][x];
          if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD)
            continue;

          // Skip unreachable resources
          if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
            continue;

          // Calculate Manhattan distance
          int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

          // Update best task if this resource is closer
          if (dist < min_dist)
          {
            min_dist = dist;
            best_task.target.x = x;
            best_task.target.y = y;
            best_task.type = cell;
          }
        }
      }
    }

    // If we found a valid task
    if (best_task.type != CELL_EMPTY)
    {
      // Calculate movement distance
      int dist = abs(villagers[i].x - best_task.target.x) +
                 abs(villagers[i].y - best_task.target.y);

      // Calculate gathering time based on resource type
      int gather_ticks = 0;
      switch (best_task.type)
      {
      case CELL_WOOD:
        gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY;
        break;
      case CELL_FOOD:
        gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY;
        break;
      case CELL_GOLD:
        gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY;
        break;
      }

      // Update total ticks (movement + gathering)
      result.total_ticks += dist + gather_ticks;

      // Update total collected resources
      result.total_collected += VILLAGER_CAPACITY;
    }
  }

  return result;
}

/**
 * Region-Based strategy: Divides map into regions and assigns resources
 * within the same region as the villager
 */
StrategyResult assign_task_region_based(Map *map, Villager *villagers, int num_villagers)
{
  StrategyResult result = {0, 0};

  // Define region dimensions (2x2 grid)
  const int region_width = MAP_WIDTH / 2;
  const int region_height = MAP_HEIGHT / 2;

  for (int i = 0; i < num_villagers; i++)
  {
    // Determine villager's region (0-3)
    int region_x = villagers[i].x / region_width;
    int region_y = villagers[i].y / region_height;
    // int region_id = region_y * 2 + region_x;

    // Calculate region boundaries
    int x_min = region_x * region_width;
    int x_max = x_min + region_width;
    int y_min = region_y * region_height;
    int y_max = y_min + region_height;

    Task best_task = {{-1, -1}, CELL_EMPTY};
    int min_dist = INT_MAX;

    // First pass: search within villager's region
    for (int y = y_min; y < y_max; y++)
    {
      for (int x = x_min; x < x_max; x++)
      {
        int cell = map->cells[y][x];
        if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD)
          continue;

        // Skip unreachable resources
        if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
          continue;

        // Calculate Manhattan distance
        int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

        // Update best task if this resource is closer
        if (dist < min_dist)
        {
          min_dist = dist;
          best_task.target.x = x;
          best_task.target.y = y;
          best_task.type = cell;
        }
      }
    }

    // If no resource found in region, search entire map
    if (best_task.type == CELL_EMPTY)
    {
      for (int y = 0; y < MAP_HEIGHT; y++)
      {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
          int cell = map->cells[y][x];
          if (cell != CELL_WOOD && cell != CELL_FOOD && cell != CELL_GOLD)
            continue;

          // Skip unreachable resources
          if (!is_reachable(map->cells, villagers[i].x, villagers[i].y, x, y))
            continue;

          // Calculate Manhattan distance
          int dist = abs(villagers[i].x - x) + abs(villagers[i].y - y);

          // Update best task if this resource is closer
          if (dist < min_dist)
          {
            min_dist = dist;
            best_task.target.x = x;
            best_task.target.y = y;
            best_task.type = cell;
          }
        }
      }
    }

    // If we found a valid task
    if (best_task.type != CELL_EMPTY)
    {
      // Calculate movement distance
      int dist = abs(villagers[i].x - best_task.target.x) +
                 abs(villagers[i].y - best_task.target.y);

      // Calculate gathering time based on resource type
      int gather_ticks = 0;
      switch (best_task.type)
      {
      case CELL_WOOD:
        gather_ticks = GATHER_TICK_PER_WOOD * VILLAGER_CAPACITY;
        break;
      case CELL_FOOD:
        gather_ticks = GATHER_TICK_PER_FOOD * VILLAGER_CAPACITY;
        break;
      case CELL_GOLD:
        gather_ticks = GATHER_TICK_PER_GOLD * VILLAGER_CAPACITY;
        break;
      }

      // Update total ticks (movement + gathering)
      result.total_ticks += dist + gather_ticks;

      // Update total collected resources
      result.total_collected += VILLAGER_CAPACITY;
    }
  }

  return result;
}