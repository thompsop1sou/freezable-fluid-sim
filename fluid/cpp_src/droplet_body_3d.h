#ifndef DROPLET_BODY_3D_H
#define DROPLET_BODY_3D_H

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/material.hpp>

#include <set>
#include <algorithm>
#include <execution>
#include <mutex>

namespace godot
{
	class DropletBody3D : public RigidBody3D
	{
		GDCLASS(DropletBody3D, RigidBody3D)

	public:
		// Let FluidServer access private/protected members
		friend class FluidServer;

		// The maximum number of nearby droplets that can be sent to the shader
		static const int NEARBY_DROPLET_COUNT_MAX = 12;

	private:
		// A struct holding information about a nearby droplet
		struct NearbyDroplet
		{
			// Properties
			DropletBody3D* body;
			float distance_squared;
			// Constructors
			NearbyDroplet();
			NearbyDroplet(DropletBody3D* p_body);
			NearbyDroplet(DropletBody3D* p_body, float p_distance_squared);
			// Comparison operators
			bool operator < (const NearbyDroplet& other_nearby_droplet) const;
			bool operator > (const NearbyDroplet& other_nearby_droplet) const;
			bool operator == (const NearbyDroplet& other_nearby_droplet) const;
			bool operator <= (const NearbyDroplet& other_nearby_droplet) const;
			bool operator >= (const NearbyDroplet& other_nearby_droplet) const;
		};

		// The mesh of this droplet
		MeshInstance3D* m_mesh_instance = nullptr;

		// A set to keep track of nearby droplets
		std::set<NearbyDroplet> m_nearby_droplets;
		std::mutex m_nearby_droplet_mutex;

		// Whether to update graphics while in-editor
		bool m_gloppy_in_editor;

		// Whether the droplet is currently frozen solid
		bool m_is_solid;

		// Keeps track of the collision of the droplet before it was solidified
		uint32_t m_pre_solid_collision_mask;
		uint32_t m_pre_solid_collision_layer;

		// The materials for the mesh when solid/liquid
		Ref<Material> m_solid_material;
		Ref<Material> m_liquid_material;

		// Whether currently in-game
		bool m_in_game;

	protected:
		// Needed for exposing stuff to Godot
		static void _bind_methods();

	public:
		// Constructor and destructor
		DropletBody3D();
		~DropletBody3D();

		// Overridden functions
		void _notification(int what);

		// Getter and setter for m_gloppy_in_editor
		bool get_gloppy_in_editor() const;
		void set_gloppy_in_editor(bool gloppy_in_editor);

		// Add or remove droplets from the set of nearby droplets
		bool add_nearby_droplet(DropletBody3D* droplet_body, float distance_squared = -1.0);
		bool remove_nearby_droplet(DropletBody3D* droplet_body);
		void clear_nearby_droplets();

		// Solidifies/liquifies the droplet
		void solidify();
		void liquify();

		// Getter for whether the droplet is frozen solid
		bool is_solid() const;

		// Getters and setters for the mesh material
		Ref<Material> get_solid_material() const;
		void set_solid_material(Ref<Material> solid_material);
		Ref<Material> get_liquid_material() const;
		void set_liquid_material(Ref<Material> liquid_material);
	
	private:
		// Notification methods
		void _on_ready();
		void _on_physics_process(double delta);
	};
}

#endif