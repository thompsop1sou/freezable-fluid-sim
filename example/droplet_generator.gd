class_name DropletGenerator
extends Node3D



# PUBLIC PROPERTIES

## How much time between each droplet generation.
@export var generation_interval: float = 0.01

## The maximum number of droplets that can be generated.
## Should not be greater than CppDropletServer.MAX_DROPLETS or CsDropletServer.MaxDroplets.
const MAX_DROPLETS = 4000

## The number of droplets to generate.
@export_range(0, MAX_DROPLETS)
var droplets_to_generate: int = 2000

## The fluid server that the generated droplets should be added to.
@export var fluid_server: FluidServer

## The file path to the scene which should be used for new droplets.
@export_file var droplet_scene_file: String



# PRIVATE PROPERTIES

# The droplet scene.
@onready var _droplet_scene: PackedScene = load(droplet_scene_file)

# Elapsed time since last droplet generation.
var _elapsed_time: float = 0.0

# The current number of droplets that have been generated.
var _num_droplets: int = 0



# PUBLIC METHODS

## Returns the number of droplets that have been generated.
func get_num_droplets() -> int:
	return _num_droplets



# PRIVATE METHODS

# Called every physics frame. 'delta' is the elapsed time since the previous frame.
func _physics_process(delta: float) -> void:
	_elapsed_time += delta
	# Check if we should generate a new droplet
	if _elapsed_time >= generation_interval and _num_droplets < droplets_to_generate:
		_elapsed_time = _elapsed_time - generation_interval
		_generate_droplet()

# Generates a droplet from a scene.
func _generate_droplet()  -> void:
	# Create the appropriate droplet type
	var droplet_node: RigidBody3D = _droplet_scene.instantiate()
	# Add the droplet to the scene
	fluid_server.add_child(droplet_node)
	droplet_node.owner = fluid_server.owner
	fluid_server.add_droplet(droplet_node)
	# Set the droplet's position
	droplet_node.position = Vector3(randf_range(-1.0, 1.0),
									randf_range(-1.0, 1.0),
									randf_range(-1.0, 1.0))
	# Increment current number of droplets
	_num_droplets += 1
