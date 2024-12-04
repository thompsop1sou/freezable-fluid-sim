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

## The current number of droplets that have been generated.
var num_droplets: int = 0



# PRIVATE PROPERTIES

# The droplet scene.
var _cpp_droplet_scene: PackedScene = preload("res://fluid/droplet.tscn")

# Elapsed time since last droplet generation.
var _elapsed_time: float = 0.0



# PRIVATE METHODS

# Called every physics frame. 'delta' is the elapsed time since the previous frame.
func _physics_process(delta: float) -> void:
	_elapsed_time += delta
	# Check if we should generate a new droplet
	if _elapsed_time >= generation_interval and num_droplets < droplets_to_generate:
		_elapsed_time = _elapsed_time - generation_interval
		# If working with droplet scenes...
		_generate_droplet_scene()
		# Increment current number of droplets
		num_droplets += 1

# Generates a droplet from a scene.
func _generate_droplet_scene()  -> void:
	# Create the appropriate droplet type
	var droplet_node: RigidBody3D = _cpp_droplet_scene.instantiate()
	# Add the droplet to the scene
	get_parent().add_child(droplet_node)
	droplet_node.owner = owner
	# Set the droplet's position
	droplet_node.position = Vector3(randf_range(-1.0, 1.0),
									randf_range(-1.0, 1.0),
									randf_range(-1.0, 1.0))
