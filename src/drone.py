"""Core Drone class for flight simulation."""

from enum import Enum
from dataclasses import dataclass
from typing import Tuple, Optional
import math


class DroneState(Enum):
    """Enumeration of possible drone states."""
    GROUNDED = "grounded"
    TAKING_OFF = "taking_off"
    HOVERING = "hovering"
    FLYING = "flying"
    LANDING = "landing"


@dataclass
class Position:
    """3D position coordinates."""
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0  # altitude
    
    def distance_to(self, other: "Position") -> float:
        """Calculate Euclidean distance to another position."""
        return math.sqrt(
            (self.x - other.x) ** 2 +
            (self.y - other.y) ** 2 +
            (self.z - other.z) ** 2
        )


@dataclass
class Velocity:
    """3D velocity components."""
    vx: float = 0.0
    vy: float = 0.0
    vz: float = 0.0
    
    @property
    def speed(self) -> float:
        """Calculate the magnitude of velocity."""
        return math.sqrt(self.vx ** 2 + self.vy ** 2 + self.vz ** 2)


class Drone:
    """
    A simulated drone with basic flight capabilities.
    
    Attributes:
        name: Identifier for the drone
        position: Current 3D position
        velocity: Current velocity vector
        state: Current operational state
        battery_level: Battery charge percentage (0-100)
        max_altitude: Maximum allowed altitude
        max_speed: Maximum horizontal speed
    """
    
    def __init__(
        self,
        name: str = "Drone-1",
        max_altitude: float = 100.0,
        max_speed: float = 10.0,
        battery_capacity: float = 100.0
    ):
        """
        Initialize a new drone.
        
        Args:
            name: Identifier for this drone
            max_altitude: Maximum flight altitude in meters
            max_speed: Maximum horizontal speed in m/s
            battery_capacity: Initial battery level (0-100)
        """
        self.name = name
        self.position = Position()
        self.velocity = Velocity()
        self.state = DroneState.GROUNDED
        self.battery_level = battery_capacity
        self.max_altitude = max_altitude
        self.max_speed = max_speed
        self._home_position = Position()
        
    def takeoff(self, target_altitude: float = 10.0) -> bool:
        """
        Initiate takeoff to a specified altitude.
        
        Args:
            target_altitude: Target altitude in meters
            
        Returns:
            True if takeoff initiated successfully, False otherwise
        """
        if self.state != DroneState.GROUNDED:
            return False
            
        if self.battery_level < 10:
            return False
            
        if target_altitude > self.max_altitude:
            target_altitude = self.max_altitude
            
        self.state = DroneState.TAKING_OFF
        self.position.z = target_altitude
        self.state = DroneState.HOVERING
        self._consume_battery(5)
        return True
        
    def land(self) -> bool:
        """
        Initiate landing sequence.
        
        Returns:
            True if landing initiated successfully, False otherwise
        """
        if self.state == DroneState.GROUNDED:
            return False
            
        self.state = DroneState.LANDING
        self.position.z = 0
        self.velocity = Velocity()
        self.state = DroneState.GROUNDED
        self._consume_battery(3)
        return True
        
    def move_to(self, x: float, y: float, z: Optional[float] = None) -> bool:
        """
        Move the drone to a specified position.
        
        Args:
            x: Target X coordinate
            y: Target Y coordinate
            z: Target altitude (optional, maintains current if not specified)
            
        Returns:
            True if movement completed, False otherwise
        """
        if self.state == DroneState.GROUNDED:
            return False
            
        if self.battery_level < 5:
            return False
            
        target_z = z if z is not None else self.position.z
        
        if target_z > self.max_altitude:
            target_z = self.max_altitude
            
        # Calculate distance for battery consumption
        target = Position(x, y, target_z)
        distance = self.position.distance_to(target)
        
        self.state = DroneState.FLYING
        self.position.x = x
        self.position.y = y
        self.position.z = target_z
        self.state = DroneState.HOVERING
        
        # Consume battery based on distance
        self._consume_battery(distance * 0.1)
        return True
        
    def hover(self) -> bool:
        """
        Enter hovering state.
        
        Returns:
            True if now hovering, False otherwise
        """
        if self.state == DroneState.GROUNDED:
            return False
            
        self.velocity = Velocity()
        self.state = DroneState.HOVERING
        self._consume_battery(0.5)
        return True
        
    def return_home(self) -> bool:
        """
        Return to the home position and land.
        
        Returns:
            True if returned home successfully, False otherwise
        """
        if self.state == DroneState.GROUNDED:
            return False
            
        self.move_to(self._home_position.x, self._home_position.y)
        return self.land()
        
    def set_home_position(self) -> None:
        """Set the current position as the home position."""
        self._home_position = Position(
            self.position.x,
            self.position.y,
            0
        )
        
    def get_telemetry(self) -> dict:
        """
        Get current drone telemetry data.
        
        Returns:
            Dictionary containing position, velocity, state, and battery info
        """
        return {
            "name": self.name,
            "position": {
                "x": self.position.x,
                "y": self.position.y,
                "z": self.position.z
            },
            "velocity": {
                "vx": self.velocity.vx,
                "vy": self.velocity.vy,
                "vz": self.velocity.vz,
                "speed": self.velocity.speed
            },
            "state": self.state.value,
            "battery_level": self.battery_level,
            "home_position": {
                "x": self._home_position.x,
                "y": self._home_position.y
            }
        }
        
    def _consume_battery(self, amount: float) -> None:
        """
        Reduce battery level by the specified amount.
        
        Args:
            amount: Amount of battery to consume
        """
        self.battery_level = max(0, self.battery_level - amount)
        
    def __repr__(self) -> str:
        """Return string representation of the drone."""
        return (
            f"Drone(name='{self.name}', "
            f"pos=({self.position.x:.1f}, {self.position.y:.1f}, {self.position.z:.1f}), "
            f"state={self.state.value}, "
            f"battery={self.battery_level:.1f}%)"
        )
