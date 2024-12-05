#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "ice_body_3d.h"

using namespace godot;

// Needed for exposing stuff to Godot
void IceBody3D::_bind_methods()
{
	// Property: force_magnitude
	ClassDB::bind_method(D_METHOD("get_frozen_droplet_radius"), &IceBody3D::get_frozen_droplet_radius);
	ClassDB::bind_method(D_METHOD("set_frozen_droplet_radius", "frozen_droplet_radius"), &IceBody3D::set_frozen_droplet_radius);
	ClassDB::add_property("IceBody3D", PropertyInfo(Variant::FLOAT, "frozen_droplet_radius"), "set_frozen_droplet_radius", "get_frozen_droplet_radius");
	
	// Methods: add_droplet and remove_droplet
	ClassDB::bind_method(D_METHOD("add_droplet", "droplet_body"), &IceBody3D::add_droplet);
	ClassDB::bind_method(D_METHOD("remove_droplet", "droplet_body"), &IceBody3D::remove_droplet);
}



// DropletCollision Methods

// Constructors
IceBody3D::DropletCollision::DropletCollision() :
	droplet_body(nullptr),
	collision_shape(nullptr)
{}
IceBody3D::DropletCollision::DropletCollision(DropletBody3D* p_droplet_body, CollisionShape3D* p_collision_shape) :
	droplet_body(p_droplet_body),
	collision_shape(p_collision_shape)
{}

// Comparison operators
bool IceBody3D::DropletCollision::operator < (const DropletCollision& other_droplet_collision) const
{
	return droplet_body < other_droplet_collision.droplet_body;
}
bool IceBody3D::DropletCollision::operator > (const DropletCollision& other_droplet_collision) const
{
	return droplet_body > other_droplet_collision.droplet_body;
}
bool IceBody3D::DropletCollision::operator == (const DropletCollision& other_droplet_collision) const
{
	return droplet_body == other_droplet_collision.droplet_body;
}
bool IceBody3D::DropletCollision::operator <= (const DropletCollision& other_droplet_collision) const
{
	return droplet_body <= other_droplet_collision.droplet_body;
}
bool IceBody3D::DropletCollision::operator >= (const DropletCollision& other_droplet_collision) const
{
	return droplet_body >= other_droplet_collision.droplet_body;
}




// Constructor and Destructor

IceBody3D::IceBody3D() :
	m_frozen_droplet_radius(0.5),
	m_frozen_droplet_collision(nullptr),
	m_droplet_collisions(),
	m_in_game(false)
{
	// Create a prototype collision shape
	m_frozen_droplet_collision = memnew(CollisionShape3D);
	m_frozen_droplet_collision->set_shape(memnew(SphereShape3D));
	m_frozen_droplet_shape = m_frozen_droplet_collision->get_shape();
	m_frozen_droplet_shape->set_radius(m_frozen_droplet_radius);
	// Center of mass should not be calculated automatically
	set_center_of_mass_mode(CENTER_OF_MASS_MODE_CUSTOM);
}

IceBody3D::~IceBody3D()
{}



// Overridden Functions

// Called when the node receives a notification of some kind.
void IceBody3D::_notification(int what)
{
	// Handle what happens when the node is about to be deleted
	if (what == NOTIFICATION_PREDELETE)
	{
		m_frozen_droplet_collision->queue_free();
	}
}



// Other Functions

// Adds/removes a droplet from the ice body

bool IceBody3D::add_droplet(DropletBody3D* new_droplet_body)
{
	// Try to find it
	auto found_location = std::find_if(m_droplet_collisions.begin(), m_droplet_collisions.end(), [new_droplet_body] (DropletCollision droplet_collision)
	{
		return droplet_collision.droplet_body == new_droplet_body;
	});

	// Not found, so add it
	if (found_location == m_droplet_collisions.end())
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
		// Create a new collision shape corresponding to the droplet
		CollisionShape3D* new_collision_shape = Object::cast_to<CollisionShape3D>(m_frozen_droplet_collision->duplicate());
		add_child(new_collision_shape);
		new_collision_shape->set_owner(get_owner());
		new_collision_shape->set_global_position(new_droplet_body->get_global_position());
		// Add them to the set
		m_droplet_collisions.push_back(DropletCollision(new_droplet_body, new_collision_shape));
		// Update the mass of this ice body
		float old_mass = m_droplet_collisions.size() > 1 ? get_mass() : 0.0;
		float droplet_mass = new_droplet_body->get_mass();
		float new_mass = old_mass + droplet_mass;
		set_mass(new_mass);
		// Update the center of mass of this ice body
		Vector3 old_center = get_center_of_mass();
		Vector3 droplet_center = new_droplet_body->get_global_position();
		Vector3 new_center = (old_center * old_mass + to_local(droplet_center) * droplet_mass) / new_mass;
		set_center_of_mass(new_center);
		// Update the velocity of this ice body (to conserve momentum)
		Vector3 old_linear_velocity = get_linear_velocity();
		Vector3 droplet_linear_velocity = new_droplet_body->get_linear_velocity();
		Vector3 new_linear_velocity = (old_linear_velocity * old_mass + droplet_linear_velocity * droplet_mass) / new_mass;
		set_linear_velocity(new_linear_velocity);
		return true;
	}
	// Found, so don't add it
	else
	{
		return false;
	}
}

bool IceBody3D::remove_droplet(DropletBody3D* old_droplet_body)
{
	// Try to find it
	auto found_location = std::find_if(m_droplet_collisions.begin(), m_droplet_collisions.end(), [old_droplet_body] (DropletCollision droplet_collision)
	{
		return droplet_collision.droplet_body == old_droplet_body;
	});

	// Couldn't find it
	if (found_location == m_droplet_collisions.end())
	{
		return false;
	}
	// Found it, so remove it
	else
	{
		DropletCollision old_droplet_collision = *found_location;
		old_droplet_collision.collision_shape->queue_free();
		m_droplet_collisions.erase(found_location);
		set_mass(get_mass() - old_droplet_collision.droplet_body->get_mass());
		return true;
	}
}

// Getters and setters for frozen droplet radius

float IceBody3D::get_frozen_droplet_radius() const
{
	return m_frozen_droplet_shape->get_radius();
}

void IceBody3D::set_frozen_droplet_radius(const float frozen_droplet_radius)
{
	m_frozen_droplet_shape->set_radius(frozen_droplet_radius);
}
