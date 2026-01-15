#!/usr/bin/env python3
"""
Drone Flight Simulation - Main Entry Point

This script demonstrates the drone simulation capabilities.
"""

from src.drone import Drone
from src.simulator import FlightSimulator


def main():
    """Run a demonstration of the drone simulation."""
    print("=" * 60)
    print("DRONE FLIGHT SIMULATION DEMO")
    print("=" * 60)
    print()
    
    # Create a flight simulator
    sim = FlightSimulator(boundary_x=500, boundary_y=500)
    
    # Create some drones
    drone1 = Drone(name="Alpha-1", max_altitude=50, max_speed=15)
    drone2 = Drone(name="Beta-2", max_altitude=100, max_speed=10)
    
    # Add drones to simulation
    sim.add_drone(drone1)
    sim.add_drone(drone2)
    
    print("Initial Status:")
    print(sim.get_status_report())
    print()
    
    # Take off
    print("\n--- Taking Off ---")
    drone1.takeoff(20)
    drone2.takeoff(30)
    print(f"{drone1.name}: {drone1}")
    print(f"{drone2.name}: {drone2}")
    
    # Set home positions
    drone1.set_home_position()
    drone2.set_home_position()
    
    # Move drones
    print("\n--- Flying to Waypoints ---")
    
    # Define waypoints for drone1
    waypoints1 = [
        (50, 50, 25),
        (100, 50, 25),
        (100, 100, 30),
        (50, 100, 25)
    ]
    
    # Execute mission for drone1
    success = sim.run_mission("Alpha-1", waypoints1)
    print(f"Alpha-1 mission completed: {success}")
    
    # Move drone2 manually
    drone2.move_to(200, 150, 40)
    print(f"Beta-2 moved to (200, 150, 40)")
    
    # Get telemetry
    print("\n--- Telemetry Data ---")
    for telemetry in sim.get_all_telemetry():
        print(f"\n{telemetry['name']}:")
        print(f"  Position: ({telemetry['position']['x']:.1f}, "
              f"{telemetry['position']['y']:.1f}, "
              f"{telemetry['position']['z']:.1f})")
        print(f"  State: {telemetry['state']}")
        print(f"  Battery: {telemetry['battery_level']:.1f}%")
    
    # Check for collisions
    print("\n--- Collision Check ---")
    collisions = sim.check_collisions()
    if collisions:
        print("WARNING: Potential collisions detected!")
        for c in collisions:
            print(f"  {c[0]} <-> {c[1]}: {c[2]:.1f}m apart")
    else:
        print("No collision risks detected.")
    
    # Return home
    print("\n--- Returning Home ---")
    sim.return_all_home()
    
    # Final status
    print("\n--- Final Status ---")
    print(sim.get_status_report())
    
    # Print flight log
    print("\n--- Flight Log ---")
    for entry in sim.get_flight_log():
        print(f"[{entry['event']}] {entry['drone']}: {entry['message']}")
    
    print("\n" + "=" * 60)
    print("DEMO COMPLETE")
    print("=" * 60)


if __name__ == "__main__":
    main()
