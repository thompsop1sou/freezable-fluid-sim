# Freezable Fluid Simulation in Godot

*Created using the [Godot](https://godotengine.org/) game engine with the [Godot Jolt](https://github.com/godot-jolt/godot-jolt) addon.*

> **Note:** This project is built out of my other repo, [Godot Fluid Sim](https://github.com/thompsop1sou/godot-fluid-sim). I made some significant changes to that project, such as removing the C# fluid server. Because of this, I decided to leave that project as it is and just create a new project to showcase the freezing ability.

This is a real-time fluid simulation that uses Godot's built-in [RigidBody3D](https://docs.godotengine.org/en/stable/classes/class_rigidbody3d.html) nodes as individual droplets of fluid. One significant contribution that this project makes is the inclusion of a "fluid server". This server applies cohesive forces to attract the droplets to each other. This helps to sell the illusion that the fluid has surface tension and viscosity. The other significant contribution from this project is the ability to freeze the droplets into ice blocks and then melt them again afterward.

[freezable_fluid_sim.webm](https://github.com/user-attachments/assets/d016daad-bf3d-4adf-ab8a-5a26cfd4562c)

*More details to follow soon...*
