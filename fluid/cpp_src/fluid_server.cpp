#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include <tuple>

#include "fluid_server.h"

using namespace godot;

// Needed for exposing stuff to Godot
void FluidServer::_bind_methods()
{
	// Property: force_magnitude
	ClassDB::bind_method(D_METHOD("get_force_magnitude"), &FluidServer::get_force_magnitude);
	ClassDB::bind_method(D_METHOD("set_force_magnitude", "force_magnitude"), &FluidServer::set_force_magnitude);
	ClassDB::add_property("FluidServer", PropertyInfo(Variant::FLOAT, "force_magnitude"), "set_force_magnitude", "get_force_magnitude");

	// Property: force_effective_distance
	ClassDB::bind_method(D_METHOD("get_force_effective_distance"), &FluidServer::get_force_effective_distance);
	ClassDB::bind_method(D_METHOD("set_force_effective_distance", "force_effective_distance"), &FluidServer::set_force_effective_distance);
	ClassDB::add_property("FluidServer", PropertyInfo(Variant::FLOAT, "force_effective_distance"), "set_force_effective_distance", "get_force_effective_distance");

	// Methods: add_droplet and remove_droplet
	ClassDB::bind_method(D_METHOD("add_droplet", "droplet_body"), &FluidServer::add_droplet);
	ClassDB::bind_method(D_METHOD("remove_droplet", "droplet_body"), &FluidServer::remove_droplet);

	// Methods: solidify, liquify, and is_solid
	ClassDB::bind_method(D_METHOD("solidify"), &FluidServer::solidify);
	ClassDB::bind_method(D_METHOD("liquify"), &FluidServer::liquify);
	ClassDB::bind_method(D_METHOD("is_solid"), &FluidServer::is_solid);
	
	// Property: ice_body_scene_path
	ClassDB::bind_method(D_METHOD("get_ice_body_scene_path"), &FluidServer::get_ice_body_scene_path);
	ClassDB::bind_method(D_METHOD("set_ice_body_scene_path", "ice_body_scene_path"), &FluidServer::set_ice_body_scene_path);
	ClassDB::add_property("FluidServer", PropertyInfo(Variant::STRING, "ice_body_scene_path", PROPERTY_HINT_FILE), "set_ice_body_scene_path", "get_ice_body_scene_path");
}



// Constructor and Destructor

FluidServer::FluidServer() :
	m_droplet_records(),
	m_force_magnitude(25.0),
	m_force_effective_distance(1.0),
	m_force_effective_distance_squared(1.0),
	m_is_solid(false),
	m_ice_bodies(),
	m_ice_body_scene_path(),
	m_ice_body_scene(),
	m_in_game(false)
{}

FluidServer::~FluidServer()
{}



// Overridden Functions

// Called when the node receives a notification of some kind.
void FluidServer::_notification(int what)
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

// Adds/removes a droplet from the server

bool FluidServer::add_droplet(DropletBody3D* new_droplet_body)
{
	// See if it already exists in the dynamic array
	std::vector<DropletRecord>::iterator found_location = std::find_if(m_droplet_records.begin(), m_droplet_records.end(),
		[new_droplet_body] (DropletRecord& droplet_record)
	{
		return droplet_record.body == new_droplet_body;
	});
	// Not found, so add it
	if (found_location == m_droplet_records.end())
	{
		// Add it as a child
		if (UtilityFunctions::is_instance_valid(new_droplet_body->get_parent()))
		{
			new_droplet_body->reparent(this, true);
			new_droplet_body->set_owner(get_owner());
		}
		else
		{
			add_child(new_droplet_body);
			new_droplet_body->set_owner(get_owner());
		}
		// Add it to the dynamic array
		DropletRecord new_droplet_record = DropletRecord();
		new_droplet_record.body = new_droplet_body;
		new_droplet_record.position = Vec3(new_droplet_body->get_global_position());
		new_droplet_record.force = Vec3::ZERO;
		m_droplet_records.push_back(new_droplet_record);
		// If the fluid is currently solid, make sure the droplet is solid also
		if (m_is_solid)
		{
			// Stop processing on the droplet
			new_droplet_body->solidify();
			// Create the ice body
			IceBody3D* ice_body = create_ice_body(new_droplet_body->get_global_position());
			// Add the droplet to it
			ice_body->add_droplet(new_droplet_body);
		}
		return true;
	}
	// Found, so don't add it
	else
	{
		return false;
	}
}

bool FluidServer::remove_droplet(DropletBody3D* old_droplet_body)
{
	// Try to find it
	std::vector<DropletRecord>::iterator found_location = std::find_if(m_droplet_records.begin(), m_droplet_records.end(),
		[&old_droplet_body] (DropletRecord& droplet_record)
	{
		return droplet_record.body == old_droplet_body;
	});
	// Couldn't find it
	if (found_location == m_droplet_records.end())
	{
		return false;
	}
	// Found it, so remove it
	else
	{
		m_droplet_records.erase(found_location);
		// If currently in a solid state...
		if (m_is_solid)
		{
			// Remove it from its ice body
			std::for_each(m_ice_bodies.begin(), m_ice_bodies.end(), [old_droplet_body] (IceBody3D* ice_body)
			{
				ice_body->remove_droplet(old_droplet_body);
			});
			// Inform nearby droplets that it is gone
			std::for_each(old_droplet_body->m_nearby_droplets.begin(), old_droplet_body->m_nearby_droplets.end(),
				[old_droplet_body] (DropletBody3D::NearbyDroplet nearby_droplet)
			{
				if (nearby_droplet.body->remove_nearby_droplet(old_droplet_body))
					UtilityFunctions::print("Removing ", old_droplet_body, " from ", nearby_droplet.body);
			});
			// Reparent it to the fluid server
			old_droplet_body->reparent(this, true);
			old_droplet_body->set_owner(get_owner());
			// Start processing on it again
			old_droplet_body->liquify();
		}
		// The removed droplet shouldn't have any nearby droplets anymore
		old_droplet_body->clear_nearby_droplets();
		return true;
	}
}

// Getters and setters for force magnitude

float FluidServer::get_force_magnitude() const
{
	return m_force_magnitude;
}

void FluidServer::set_force_magnitude(const float force_magnitude)
{
	m_force_magnitude = force_magnitude;
}

// Getters and setters for force effective distance

float FluidServer::get_force_effective_distance() const
{
	return m_force_effective_distance;
}

void FluidServer::set_force_effective_distance(const float force_effective_distance)
{
	if (force_effective_distance < 0)
		m_force_effective_distance = 0;
	else
		m_force_effective_distance = force_effective_distance;
	m_force_effective_distance_squared = m_force_effective_distance * m_force_effective_distance;
}

// Solidifies/liquifies the droplets in this server

void FluidServer::solidify()
{
	// Return early if already solid
	if (m_is_solid)
		return;

	// Create the array of droplet sets and their average positions
	std::vector<DropletSet> droplet_sets;
	std::vector<Vec3> droplet_set_positions;

	// First loop to stop processing and group droplets together into a set
	std::for_each(m_droplet_records.begin(), m_droplet_records.end(), [this, &droplet_sets, &droplet_set_positions] (DropletRecord& droplet_record)
	{
		// Stop processing
		droplet_record.body->solidify();
		// Test if it's already in a set
		std::vector<DropletSet>::iterator found_set_iter = std::find_if(droplet_sets.begin(), droplet_sets.end(), [&droplet_record] (DropletSet& droplet_set)
		{
			return droplet_set.find(droplet_record.body) != droplet_set.end();
		});
		// Not yet in a set...
		if (found_set_iter == droplet_sets.end())
		{
			// Add it to a new set (and its nearby droplets recursively)
			DropletSet new_droplet_set = DropletSet();
			Vec3 new_droplet_set_position = Vec3::ZERO;
			add_droplet_to_set(droplet_record.body, new_droplet_set, new_droplet_set_position);
			// Add that set to the dynamic array of sets
			droplet_sets.push_back(new_droplet_set);
			droplet_set_positions.push_back(new_droplet_set_position);
		}
	});

	// Second loop to create an ice block for each droplet set
	std::for_each(droplet_sets.begin(), droplet_sets.end(), [this, &droplet_sets, &droplet_set_positions] (DropletSet& droplet_set)
	{
		// Get the index of the current droplet set
		int index = &droplet_set - &droplet_sets.front();
		// Create a new ice body
		IceBody3D* ice_body = create_ice_body(Vector3(droplet_set_positions[index]));
		// Add the droplets to it
		std::for_each(droplet_set.begin(), droplet_set.end(), [&ice_body] (DropletBody3D* droplet_body)
		{
			ice_body->add_droplet(droplet_body);
		});
	});

	// Mark it as frozen
	m_is_solid = true;
}

void FluidServer::liquify()
{
	// Return early if already liquid
	if (!m_is_solid)
		return;

	// Loop over each of the ice blocks
	std::for_each(m_ice_bodies.begin(), m_ice_bodies.end(), [this] (IceBody3D* ice_body)
	{
		// Remove the droplets from this ice body
		std::for_each(ice_body->m_droplet_collisions.begin(), ice_body->m_droplet_collisions.end(),
			[this] (IceBody3D::DropletCollision& droplet_collision)
		{
			droplet_collision.droplet_body->reparent(this);
			droplet_collision.droplet_body->liquify();
		});
		// Delete the ice body
		ice_body->queue_free();
	});

	// Clear the ice body array
	m_ice_bodies.clear();

	// Mark the server as liquid
	m_is_solid = false;
}

// Getter for whether the droplets are frozen solid
bool FluidServer::is_solid() const
{
	return m_is_solid;
}

// Getters and setters for ice body scene path

String FluidServer::get_ice_body_scene_path() const
{
	return m_ice_body_scene_path;
}

void FluidServer::set_ice_body_scene_path(const String ice_body_scene_path)
{
	m_ice_body_scene_path = ice_body_scene_path;
}

// Adds droplets recursively to a set (helper for solidify())
void FluidServer::add_droplet_to_set(DropletBody3D* droplet_body, DropletSet& droplet_set, Vec3& droplet_set_position)
{
	// If droplet has not been added...
	if (droplet_set.find(droplet_body) == droplet_set.end())
	{
		// Add it to the set
		droplet_set.insert(droplet_body);
		// Update the average position
		droplet_set_position = (droplet_set_position * (droplet_set.size() - 1.0) + droplet_body->get_global_position()) / droplet_set.size();
		// Recurse over the nearby droplets
		std::for_each(droplet_body->m_nearby_droplets.begin(), droplet_body->m_nearby_droplets.end(),
			[this, &droplet_set, &droplet_set_position] (DropletBody3D::NearbyDroplet next_nearby_droplet)
		{
			add_droplet_to_set(next_nearby_droplet.body, droplet_set, droplet_set_position);
		});
	}
}

// Creates a new ice body at a given position, adds it to the array of ice bodies, and returns it
IceBody3D* FluidServer::create_ice_body(Vector3 ice_body_global_position)
{
	IceBody3D* ice_body = Object::cast_to<IceBody3D>(m_ice_body_scene->instantiate());
	add_child(ice_body);
	ice_body->set_owner(get_owner());
	ice_body->set_global_position(ice_body_global_position);
	m_ice_bodies.push_back(ice_body);
	return ice_body;
}



// Notification Methods

// Called when the node enters the scene tree for the first time.
void FluidServer::_on_ready()
{
	// Determine whether the game is running
	m_in_game = !Engine::get_singleton()->is_editor_hint();
	// Set this to process sooner than everything else
	set_physics_process_priority(-1);
	// Load up the ice body scene
	if (m_in_game)
	{
		m_ice_body_scene = ResourceLoader::get_singleton()->load(m_ice_body_scene_path);
	}
}

// Called every physics frame. 'delta' is the elapsed time since the previous frame.
void FluidServer::_on_physics_process(double delta)
{
	// Only run if in game and not currently solid
	if (m_in_game && !m_is_solid)
	{
		// Get the current position of each droplet
		std::for_each(m_droplet_records.begin(), m_droplet_records.end(), [this] (DropletRecord& droplet_record)
		{
			droplet_record.position = Vec3(droplet_record.body->get_global_position());
			droplet_record.body->clear_nearby_droplets();
		});
		// Sum up the forces by looping over pairs of droplets
		// Outer loop to get first droplet
		std::for_each(std::execution::par, m_droplet_records.begin(), m_droplet_records.end(), [this] (DropletRecord& droplet_record_a)
		{
			// Get an iterator to the first droplet
			std::vector<DropletRecord>::iterator droplet_a_iter = m_droplet_records.begin() + (&droplet_record_a - &m_droplet_records.front());
			// Inner loop to get second droplet
			std::for_each(std::execution::par, droplet_a_iter + 1, m_droplet_records.end(), [this, &droplet_record_a] (DropletRecord& droplet_record_b)
			{
				// Test if the droplets are close enough
				float distance_squared = droplet_record_a.position.distance_squared(droplet_record_b.position);
				if (distance_squared < m_force_effective_distance_squared)
				{
					// Apply cohesive forces
					Vec3 force_direction = (droplet_record_a.position - droplet_record_b.position).normalized();
					droplet_record_a.mutex->lock();
					droplet_record_a.force += -m_force_magnitude * force_direction;
					droplet_record_a.mutex->unlock();
					droplet_record_b.mutex->lock();
					droplet_record_b.force += +m_force_magnitude * force_direction;
					droplet_record_b.mutex->unlock();
					// Inform the droplets that they are near each other
					droplet_record_a.body->add_nearby_droplet(droplet_record_b.body, distance_squared);
					droplet_record_b.body->add_nearby_droplet(droplet_record_a.body, distance_squared);
				}
			});
		});
		// Apply the forces for each droplet
		std::for_each(std::execution::par, m_droplet_records.begin(), m_droplet_records.end(), [this] (DropletRecord& droplet_record)
		{
			droplet_record.body->apply_central_force(Vector3(droplet_record.force));
			droplet_record.force = Vec3::ZERO;
		});
	}
}
