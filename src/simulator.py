"""Simple drone flight simulator."""

from typing import List, Optional
from .drone import Drone, DroneState, Position


class FlightSimulator:
    """
    A simple flight simulation environment for drones.
    
    Manages one or more drones and provides simulation capabilities.
    """
    
    def __init__(self, boundary_x: float = 1000, boundary_y: float = 1000):
        """
        Initialize the simulator.
        
        Args:
            boundary_x: Maximum X coordinate boundary
            boundary_y: Maximum Y coordinate boundary
        """
        self.drones: List[Drone] = []
        self.boundary_x = boundary_x
        self.boundary_y = boundary_y
        self.flight_log: List[dict] = []
        
    def add_drone(self, drone: Drone) -> None:
        """
        Add a drone to the simulation.
        
        Args:
            drone: Drone instance to add
        """
        self.drones.append(drone)
        self._log_event(drone.name, "added", "Drone added to simulation")
        
    def remove_drone(self, name: str) -> Optional[Drone]:
        """
        Remove a drone from the simulation by name.
        
        Args:
            name: Name of the drone to remove
            
        Returns:
            The removed drone, or None if not found
        """
        for i, drone in enumerate(self.drones):
            if drone.name == name:
                removed = self.drones.pop(i)
                self._log_event(name, "removed", "Drone removed from simulation")
                return removed
        return None
        
    def get_drone(self, name: str) -> Optional[Drone]:
        """
        Get a drone by name.
        
        Args:
            name: Name of the drone to find
            
        Returns:
            The drone if found, None otherwise
        """
        for drone in self.drones:
            if drone.name == name:
                return drone
        return None
        
    def get_all_telemetry(self) -> List[dict]:
        """
        Get telemetry data from all drones.
        
        Returns:
            List of telemetry dictionaries for all drones
        """
        return [drone.get_telemetry() for drone in self.drones]
        
    def is_within_bounds(self, position: Position) -> bool:
        """
        Check if a position is within simulation boundaries.
        
        Args:
            position: Position to check
            
        Returns:
            True if within bounds, False otherwise
        """
        return (
            0 <= position.x <= self.boundary_x and
            0 <= position.y <= self.boundary_y
        )
        
    def check_collisions(self, min_distance: float = 5.0) -> List[tuple]:
        """
        Check for potential collisions between drones.
        
        Args:
            min_distance: Minimum safe distance between drones
            
        Returns:
            List of tuples containing drone names that are too close
        """
        collisions = []
        for i, drone1 in enumerate(self.drones):
            for drone2 in self.drones[i + 1:]:
                distance = drone1.position.distance_to(drone2.position)
                if distance < min_distance:
                    collisions.append((drone1.name, drone2.name, distance))
        return collisions
        
    def run_mission(self, drone_name: str, waypoints: List[tuple]) -> bool:
        """
        Execute a flight mission through a series of waypoints.
        
        Args:
            drone_name: Name of the drone to fly
            waypoints: List of (x, y, z) coordinate tuples
            
        Returns:
            True if mission completed, False otherwise
        """
        drone = self.get_drone(drone_name)
        if not drone:
            return False
            
        self._log_event(drone_name, "mission_start", f"Starting mission with {len(waypoints)} waypoints")
        
        for i, waypoint in enumerate(waypoints):
            x, y, z = waypoint
            target = Position(x, y, z)
            
            if not self.is_within_bounds(target):
                self._log_event(drone_name, "mission_error", f"Waypoint {i} out of bounds")
                return False
                
            if not drone.move_to(x, y, z):
                self._log_event(drone_name, "mission_error", f"Failed to reach waypoint {i}")
                return False
                
            self._log_event(drone_name, "waypoint_reached", f"Reached waypoint {i}: ({x}, {y}, {z})")
            
        self._log_event(drone_name, "mission_complete", "Mission completed successfully")
        return True
        
    def emergency_land_all(self) -> None:
        """Command all drones to land immediately."""
        for drone in self.drones:
            if drone.state != DroneState.GROUNDED:
                drone.land()
                self._log_event(drone.name, "emergency_land", "Emergency landing executed")
                
    def return_all_home(self) -> None:
        """Command all drones to return to their home positions."""
        for drone in self.drones:
            if drone.state != DroneState.GROUNDED:
                drone.return_home()
                self._log_event(drone.name, "return_home", "Returning to home position")
                
    def _log_event(self, drone_name: str, event_type: str, message: str) -> None:
        """
        Log a simulation event.
        
        Args:
            drone_name: Name of the drone involved
            event_type: Type of event
            message: Event description
        """
        self.flight_log.append({
            "drone": drone_name,
            "event": event_type,
            "message": message
        })
        
    def get_flight_log(self) -> List[dict]:
        """
        Get the complete flight log.
        
        Returns:
            List of logged events
        """
        return self.flight_log
        
    def clear_flight_log(self) -> None:
        """Clear all logged events."""
        self.flight_log = []
        
    def get_status_report(self) -> str:
        """
        Generate a status report for all drones.
        
        Returns:
            Formatted status report string
        """
        lines = ["=" * 50]
        lines.append("FLIGHT SIMULATOR STATUS REPORT")
        lines.append("=" * 50)
        lines.append(f"Active Drones: {len(self.drones)}")
        lines.append(f"Boundary: {self.boundary_x} x {self.boundary_y}")
        lines.append("-" * 50)
        
        for drone in self.drones:
            lines.append(f"\n{drone.name}:")
            lines.append(f"  State: {drone.state.value}")
            lines.append(f"  Position: ({drone.position.x:.1f}, {drone.position.y:.1f}, {drone.position.z:.1f})")
            lines.append(f"  Battery: {drone.battery_level:.1f}%")
            
        lines.append("\n" + "=" * 50)
        return "\n".join(lines)
