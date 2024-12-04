#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "droplet_body_3d.h"

using namespace godot;

// Needed for exposing stuff to Godot
void DropletBody3D::_bind_methods()
{
	// Property: gloppy_in_editor
	ClassDB::bind_method(D_METHOD("get_gloppy_in_editor"), &DropletBody3D::get_gloppy_in_editor);
	ClassDB::bind_method(D_METHOD("set_gloppy_in_editor", "mesh_instance"), &DropletBody3D::set_gloppy_in_editor);
	ClassDB::add_property("DropletBody3D", PropertyInfo(Variant::BOOL, "gloppy_in_editor"), "set_gloppy_in_editor", "get_gloppy_in_editor");

	// Methods: add_nearby_droplet, remove_nearby_droplet, and clear_nearby_droplets
	ClassDB::bind_method(D_METHOD("add_nearby_droplet", "new_droplet_body", "new_distance_squared"), &DropletBody3D::add_nearby_droplet, DEFVAL(-1.0));
	ClassDB::bind_method(D_METHOD("remove_nearby_droplet", "old_droplet_body"), &DropletBody3D::remove_nearby_droplet);
	ClassDB::bind_method(D_METHOD("clear_nearby_droplets"), &DropletBody3D::clear_nearby_droplets);

	// Methods: solidify, liquify, and is_solid
	ClassDB::bind_method(D_METHOD("solidify"), &DropletBody3D::solidify);
	ClassDB::bind_method(D_METHOD("liquify"), &DropletBody3D::liquify);
	ClassDB::bind_method(D_METHOD("is_solid"), &DropletBody3D::is_solid);

	// Property: solid_material
	ClassDB::bind_method(D_METHOD("get_solid_material"), &DropletBody3D::get_solid_material);
	ClassDB::bind_method(D_METHOD("set_solid_material", "solid_material"), &DropletBody3D::set_solid_material);
	ClassDB::add_property("DropletBody3D", PropertyInfo(Variant::OBJECT, "solid_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_solid_material", "get_solid_material");

	// Property: liquid_material
	ClassDB::bind_method(D_METHOD("get_liquid_material"), &DropletBody3D::get_liquid_material);
	ClassDB::bind_method(D_METHOD("set_liquid_material", "liquid_material"), &DropletBody3D::set_liquid_material);
	ClassDB::add_property("DropletBody3D", PropertyInfo(Variant::OBJECT, "liquid_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_liquid_material", "get_liquid_material");
}



// NearbyDroplet Methods

// Constructors
DropletBody3D::NearbyDroplet::NearbyDroplet() :
	body(nullptr),
	distance_squared(-1.0)
{}
DropletBody3D::NearbyDroplet::NearbyDroplet(DropletBody3D* p_body) :
	body(p_body),
	distance_squared(-1.0)
{}
DropletBody3D::NearbyDroplet::NearbyDroplet(DropletBody3D* p_body, float p_distance_squared) :
	body(p_body),
	distance_squared(p_distance_squared)
{}

// Comparison operators
bool DropletBody3D::NearbyDroplet::operator < (const NearbyDroplet& other_nearby_droplet) const
{
	if (distance_squared == other_nearby_droplet.distance_squared)
	{
		return body < other_nearby_droplet.body;
	}
	else
	{
		return distance_squared < other_nearby_droplet.distance_squared;
	}
}
bool DropletBody3D::NearbyDroplet::operator > (const NearbyDroplet& other_nearby_droplet) const
{
	if (distance_squared == other_nearby_droplet.distance_squared)
	{
		return body > other_nearby_droplet.body;
	}
	else
	{
		return distance_squared > other_nearby_droplet.distance_squared;
	}
}
bool DropletBody3D::NearbyDroplet::operator == (const NearbyDroplet& other_nearby_droplet) const
{
	return distance_squared == other_nearby_droplet.distance_squared && body == other_nearby_droplet.body;
}
bool DropletBody3D::NearbyDroplet::operator <= (const NearbyDroplet& other_nearby_droplet) const
{
	return *this < other_nearby_droplet || *this == other_nearby_droplet;
}
bool DropletBody3D::NearbyDroplet::operator >= (const NearbyDroplet& other_nearby_droplet) const
{
	return *this > other_nearby_droplet || *this == other_nearby_droplet;
}



// Constructor and Destructor

DropletBody3D::DropletBody3D() :
	m_mesh_instance(nullptr),
	m_gloppy_in_editor(false),
	m_nearby_droplets(),
	m_nearby_droplet_mutex(),
	m_is_solid(false),
	m_pre_solid_collision_mask(0),
	m_pre_solid_collision_layer(0),
	m_solid_material(nullptr),
	m_liquid_material(nullptr),
	m_in_game(false)
{}

DropletBody3D::~DropletBody3D()
{}



// Overridden Functions

// Called when the node receives a notification of some kind.
void DropletBody3D::_notification(int what)
{
	switch (what)
	{
		// Handle when the node enters the scene tree for the first time.
		case NOTIFICATION_READY:
			_on_ready();
			set_physics_process(true);
			break;
		// Handle the physics frame.
		case NOTIFICATION_PHYSICS_PROCESS:
			_on_physics_process(get_physics_process_delta_time());
			break;
	}
}



// Other Functions

// Get whether gloppy in editor.
bool DropletBody3D::get_gloppy_in_editor() const
{
	return m_gloppy_in_editor;
}

// Set whether gloppy in editor.
void DropletBody3D::set_gloppy_in_editor(bool gloppy_in_editor)
{
	m_gloppy_in_editor = gloppy_in_editor;
}

// Adds a nearby droplet to the set.
bool DropletBody3D::add_nearby_droplet(DropletBody3D* new_droplet_body, float new_distance_squared)
{
	bool result = false;

	// Lock for thread safety
	m_nearby_droplet_mutex.lock();

	// Construct the new droplet
	NearbyDroplet new_nearby_droplet = NearbyDroplet(new_droplet_body, new_distance_squared);
	if (new_distance_squared < 0.0)
	{
		new_nearby_droplet.distance_squared = get_global_position().distance_squared_to(new_droplet_body->get_global_position());
	}

	// If the number of nearby droplets is already at max...
	if (m_nearby_droplets.size() >= NEARBY_DROPLET_COUNT_MAX)
	{
		// Find the furthest droplet
		auto furthest_droplet_iter = --(m_nearby_droplets.end());
		// Only add the new droplet if it is closer than the furthest droplet
		if (new_nearby_droplet < *furthest_droplet_iter)
		{
			m_nearby_droplets.erase(furthest_droplet_iter);
			m_nearby_droplets.insert(new_nearby_droplet);
			result = true;
		}
	}
	// Otherwise, if not at max, we can just add the new droplet
	else
	{
		m_nearby_droplets.insert(new_nearby_droplet);
		result = true;
	}

	// Unlock for thread safety
	m_nearby_droplet_mutex.unlock();

	return result;
}

// Removes a nearby droplet from the set.
bool DropletBody3D::remove_nearby_droplet(DropletBody3D* old_droplet_body)
{
	bool result = false;

	// Lock for thread safety
	m_nearby_droplet_mutex.lock();

	// Search for the nearby droplet that has the same body
	auto found_nearby_droplet_iter = std::find_if(m_nearby_droplets.begin(), m_nearby_droplets.end(), [old_droplet_body] (NearbyDroplet nearby_droplet)
	{
		return nearby_droplet.body == old_droplet_body;
	});

	// If found, remove it
	if (found_nearby_droplet_iter != m_nearby_droplets.end())
	{
		m_nearby_droplets.erase(found_nearby_droplet_iter);
		result = true;
	}

	// Unlock for thread safety
	m_nearby_droplet_mutex.unlock();

	return result;
}

// Clears out the set of nearby droplets.
void DropletBody3D::clear_nearby_droplets()
{
	// Lock for thread safety
	m_nearby_droplet_mutex.lock();

	// Clear out the whole array
	m_nearby_droplets.clear();

	// Unlock for thread safety
	m_nearby_droplet_mutex.unlock();
}

// Solidifies/liquifies the droplet

void DropletBody3D::solidify()
{
	// Return early if already frozen
	if (m_is_solid)
		return;

	// Freeze/solidify
	m_is_solid = true;
	set_freeze_enabled(true);

	// Set collisions
	m_pre_solid_collision_mask = get_collision_mask();
	m_pre_solid_collision_layer = get_collision_layer();
	set_collision_mask(0);
	set_collision_layer(0);

	// Set mesh
	if (UtilityFunctions::is_instance_valid(m_mesh_instance))
	{
		m_mesh_instance->set_material_override(m_solid_material);
	}
}

void DropletBody3D::liquify()
{
	// Return early if already melted
	if (!m_is_solid)
		return;

	// Unfreeze/liquify
	m_is_solid = false;
	set_freeze_enabled(false);

	// Set collisions
	set_collision_mask(m_pre_solid_collision_mask);
	set_collision_layer(m_pre_solid_collision_layer);

	// Set mesh
	if (UtilityFunctions::is_instance_valid(m_mesh_instance))
	{
		m_mesh_instance->set_material_override(m_liquid_material);
	}
}

// Getter for whether the droplet is frozen solid
bool DropletBody3D::is_solid() const
{
	return m_is_solid;
}

// Getter and setter for the solid mesh material

Ref<Material> DropletBody3D::get_solid_material() const
{
	return m_solid_material;
}

void DropletBody3D::set_solid_material(Ref<Material> solid_material)
{
	m_solid_material = solid_material;
}

// Getter and setter for the liquid mesh material

Ref<Material> DropletBody3D::get_liquid_material() const
{
	return m_liquid_material;
}

void DropletBody3D::set_liquid_material(Ref<Material> liquid_material)
{
	m_liquid_material = liquid_material;
}



// Notification Methods

// Called when the node enters the scene tree for the first time.
void DropletBody3D::_on_ready()
{
	// Determine whether the game is running
	m_in_game = !Engine::get_singleton()->is_editor_hint();
	// Find the mesh
	m_mesh_instance = Object::cast_to<MeshInstance3D>(find_children("*", "MeshInstance3D").front());
	// Set the material on the mesh
	if (UtilityFunctions::is_instance_valid(m_mesh_instance))
	{
		m_mesh_instance->set_material_override(m_is_solid ? m_solid_material : m_liquid_material);
	}
	// Cound not find mesh
	else
	{
		UtilityFunctions::printerr("Could not find mesh for ", this);
	}
}

// Called every physics frame. 'delta' is the elapsed time since the previous frame.
void DropletBody3D::_on_physics_process(double delta)
{
	// Only run if in game and the mesh is valid
	if ((m_in_game || m_gloppy_in_editor) && UtilityFunctions::is_instance_valid(m_mesh_instance))
	{
		// Lock for thread safety
		m_nearby_droplet_mutex.lock();

		// Set the shader uniform for the number of droplets (add one for this current droplet)
		m_mesh_instance->set_instance_shader_parameter(StringName("droplet_count"), m_nearby_droplets.size() + 1);

		// Tell the shader about the nearby droplets
		int i = 1;
		std::for_each(m_nearby_droplets.begin(), m_nearby_droplets.end(), [this, &i] (NearbyDroplet nearby_droplet)
		{
			// Get the uniform name for the position of droplet i
			String uniform_name = String("position_") + UtilityFunctions::str(i);
			i++;
			// Set the uniform for the position of droplet i
			Vector3 global_position = nearby_droplet.body->get_global_position();
			m_mesh_instance->set_instance_shader_parameter(StringName(uniform_name), global_position);
		});

		// Unlock for thread safety
		m_nearby_droplet_mutex.unlock();
	}
}