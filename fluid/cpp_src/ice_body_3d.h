#ifndef ICE_BODY_3D_H
#define ICE_BODY_3D_H

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>

#include <vector>
#include <algorithm>

#include "droplet_body_3d.h"

namespace godot
{
	class IceBody3D : public RigidBody3D
	{
		GDCLASS(IceBody3D, RigidBody3D)

	public:
		// Let FluidServer access private/protected members
		friend class FluidServer;

	private:
		// A struct holding information about a droplet and its corresponding collision
		struct DropletCollision
		{
			// Properties
			DropletBody3D* droplet_body;
			CollisionShape3D* collision_shape;
			// Constructors
			DropletCollision();
			DropletCollision(DropletBody3D* p_droplet_body, CollisionShape3D* p_collision_shape);
			// Comparison operators
			bool operator < (const DropletCollision& other_droplet_collision) const;
			bool operator > (const DropletCollision& other_droplet_collision) const;
			bool operator == (const DropletCollision& other_droplet_collision) const;
			bool operator <= (const DropletCollision& other_droplet_collision) const;
			bool operator >= (const DropletCollision& other_droplet_collision) const;
		};

		// The collision shape for each droplet in the ice body
		float m_frozen_droplet_radius;
		CollisionShape3D* m_frozen_droplet_collision;
		Ref<SphereShape3D> m_frozen_droplet_shape;

		// A dynamic array of the droplets in the ice body along with their corresponding collisions
		std::vector<DropletCollision> m_droplet_collisions;

		// Whether currently in-game
		bool m_in_game;

	protected:
		// Needed for exposing stuff to Godot
		static void _bind_methods();

	public:
		// Constructor and destructor
		IceBody3D();
		~IceBody3D();

		// Overridden functions
		void _notification(int what);

		// Getter and setter for frozen droplet radius
		float get_frozen_droplet_radius() const;
		void set_frozen_droplet_radius(const float frozen_droplet_radius);

		// Adds/removes a droplet from the ice body
		bool add_droplet(DropletBody3D* new_droplet_body);
		bool remove_droplet(DropletBody3D* old_droplet_body);
	};
}

#endif