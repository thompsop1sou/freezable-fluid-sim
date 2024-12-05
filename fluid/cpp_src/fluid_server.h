#ifndef FLUID_SERVER_H
#define FLUID_SERVER_H

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include <vector>
#include <execution>
#include <mutex>
#include <unordered_set>
#include <memory>

#include "vec3.h"
#include "droplet_body_3d.h"
#include "ice_body_3d.h"

namespace godot
{
	class FluidServer : public Node3D
	{
		GDCLASS(FluidServer, Node3D)

	private:
		// A set of droplets
		typedef std::unordered_set<DropletBody3D*> DropletSet;

		// A struct to hold information about each droplet
		struct DropletRecord
		{
			// Properties
			DropletBody3D* body;
			Vec3 position;
			Vec3 force;
			std::shared_ptr<std::mutex> mutex; // std::mutex cannot be copied, so instead we pass around a pointer to it
			// Constructor
			DropletRecord() : body(nullptr), position(0.0), force(0.0), mutex(nullptr)
			{
				mutex = std::make_shared<std::mutex>();
			}
		};

		// A dynamic array of Droplet structs
		std::vector<DropletRecord> m_droplet_records;

		// The magnitude of the attraction force
		float m_force_magnitude;

		// The distance that the attraction force is effective over
		float m_force_effective_distance;
		float m_force_effective_distance_squared;

		// Whether the droplets are currently frozen solid
		bool m_is_solid;

		// A dynamic array of ice bodies for when solid
		std::vector<IceBody3D*> m_ice_bodies;

		// The scene that should be used for ice bodies when freezing
		String m_ice_body_scene_path;
		Ref<PackedScene> m_ice_body_scene;

		// Whether currently in-game
		bool m_in_game;

	protected:
		// Needed for exposing stuff to Godot
		static void _bind_methods();

	public:
		// Constructor and destructor
		FluidServer();
		~FluidServer();

		// Overridden functions
		void _notification(int what);

		// Adds/removes a droplet from the server
		bool add_droplet(DropletBody3D* new_droplet_body);
		bool remove_droplet(DropletBody3D* old_droplet_body);

		// Getter and setter for force magnitude
		float get_force_magnitude() const;
		void set_force_magnitude(const float force_magnitude);

		// Getter and setter for force effective distance
		float get_force_effective_distance() const;
		void set_force_effective_distance(const float force_effective_distance);

		// Solidifies/liquifies the droplets in this server
		void solidify();
		void liquefy();

		// Getter and setter for ice body scene path
		String get_ice_body_scene_path() const;
		void set_ice_body_scene_path(const String ice_body_scene_path);

		// Getter for whether the droplets are frozen solid
		bool is_solid() const;

	private:
		// Adds droplets recursively to a set (helper for freeze())
		void add_droplet_to_set(DropletBody3D* droplet_body, DropletSet& droplet_set, Vec3& droplet_set_position);

		// Creates a new ice body at a given position, adds it to the array of ice bodies, and returns it
		IceBody3D* create_ice_body(Vector3 ice_body_global_position = Vector3());

		// Notification methods
		void _on_ready();
		void _on_physics_process(double delta);
	};
}

#endif