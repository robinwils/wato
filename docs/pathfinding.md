# Notes

- https://www.redblobgames.com/pathfinding/tower-defense
Interesting article on pathfinding in a TD, basically in wato we have
one source one destination at first (maybe we could have multiple sources at some point)
so A* is the way to go.

# Graph

The way to build the graph would be to have a fine grid over the real world
and update cell walkability when adding/removing towers using RP3D's overlap/trigger
event listener objects.

## Implementation

https://www.redblobgames.com/pathfinding/a-star/implementation.html#cpp

This link shows a simple implementation of a MxN graph with obstacles as a set of (x,y).
This seems like the best way to easily manage the graph.

the path is not in the graph struct itself but passed as reference to the
pathfinding algorithm.

### ECS relationships

We need to keep the entities that are blocking the path inside the
obstacle set in order to attack towers if the maze is totally blocking the destination
node.
