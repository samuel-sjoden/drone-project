"""Unit tests for the Drone class."""

import pytest
from src.drone import Drone, DroneState, Position, Velocity


class TestPosition:
    """Tests for the Position dataclass."""
    
    def test_default_position(self):
        """Test default position is at origin."""
        pos = Position()
        assert pos.x == 0.0
        assert pos.y == 0.0
        assert pos.z == 0.0
        
    def test_custom_position(self):
        """Test creating position with custom coordinates."""
        pos = Position(10.0, 20.0, 30.0)
        assert pos.x == 10.0
        assert pos.y == 20.0
        assert pos.z == 30.0
        
    def test_distance_to_same_point(self):
        """Test distance to same point is zero."""
        pos = Position(5.0, 5.0, 5.0)
        assert pos.distance_to(pos) == 0.0
        
    def test_distance_to_other_point(self):
        """Test distance calculation between two points."""
        pos1 = Position(0.0, 0.0, 0.0)
        pos2 = Position(3.0, 4.0, 0.0)
        assert pos1.distance_to(pos2) == 5.0  # 3-4-5 triangle


class TestVelocity:
    """Tests for the Velocity dataclass."""
    
    def test_default_velocity(self):
        """Test default velocity is zero."""
        vel = Velocity()
        assert vel.vx == 0.0
        assert vel.vy == 0.0
        assert vel.vz == 0.0
        
    def test_speed_calculation(self):
        """Test speed property calculation."""
        vel = Velocity(3.0, 4.0, 0.0)
        assert vel.speed == 5.0


class TestDrone:
    """Tests for the Drone class."""
    
    def test_initial_state(self):
        """Test drone starts in grounded state."""
        drone = Drone()
        assert drone.state == DroneState.GROUNDED
        assert drone.position.z == 0.0
        assert drone.battery_level == 100.0
        
    def test_custom_initialization(self):
        """Test drone with custom parameters."""
        drone = Drone(name="TestDrone", max_altitude=50.0, max_speed=5.0, battery_capacity=80.0)
        assert drone.name == "TestDrone"
        assert drone.max_altitude == 50.0
        assert drone.max_speed == 5.0
        assert drone.battery_level == 80.0
        
    def test_takeoff(self):
        """Test successful takeoff."""
        drone = Drone()
        result = drone.takeoff(10.0)
        assert result is True
        assert drone.state == DroneState.HOVERING
        assert drone.position.z == 10.0
        
    def test_takeoff_already_flying(self):
        """Test takeoff fails when already flying."""
        drone = Drone()
        drone.takeoff(10.0)
        result = drone.takeoff(20.0)
        assert result is False
        
    def test_takeoff_low_battery(self):
        """Test takeoff fails with low battery."""
        drone = Drone(battery_capacity=5.0)
        result = drone.takeoff(10.0)
        assert result is False
        assert drone.state == DroneState.GROUNDED
        
    def test_takeoff_altitude_limit(self):
        """Test takeoff respects max altitude."""
        drone = Drone(max_altitude=50.0)
        drone.takeoff(100.0)
        assert drone.position.z == 50.0
        
    def test_land(self):
        """Test successful landing."""
        drone = Drone()
        drone.takeoff(20.0)
        result = drone.land()
        assert result is True
        assert drone.state == DroneState.GROUNDED
        assert drone.position.z == 0.0
        
    def test_land_already_grounded(self):
        """Test landing fails when already grounded."""
        drone = Drone()
        result = drone.land()
        assert result is False
        
    def test_move_to(self):
        """Test movement to position."""
        drone = Drone()
        drone.takeoff(10.0)
        result = drone.move_to(50.0, 50.0, 15.0)
        assert result is True
        assert drone.position.x == 50.0
        assert drone.position.y == 50.0
        assert drone.position.z == 15.0
        
    def test_move_when_grounded(self):
        """Test movement fails when grounded."""
        drone = Drone()
        result = drone.move_to(50.0, 50.0, 10.0)
        assert result is False
        
    def test_move_maintains_altitude(self):
        """Test movement maintains altitude when not specified."""
        drone = Drone()
        drone.takeoff(20.0)
        drone.move_to(30.0, 40.0)
        assert drone.position.z == 20.0
        
    def test_hover(self):
        """Test hover state."""
        drone = Drone()
        drone.takeoff(10.0)
        drone.velocity.vx = 5.0
        result = drone.hover()
        assert result is True
        assert drone.state == DroneState.HOVERING
        assert drone.velocity.vx == 0.0
        
    def test_hover_when_grounded(self):
        """Test hover fails when grounded."""
        drone = Drone()
        result = drone.hover()
        assert result is False
        
    def test_return_home(self):
        """Test return to home position."""
        drone = Drone()
        drone.takeoff(10.0)
        drone.set_home_position()
        drone.move_to(100.0, 100.0, 20.0)
        result = drone.return_home()
        assert result is True
        assert drone.state == DroneState.GROUNDED
        assert drone.position.x == 0.0
        assert drone.position.y == 0.0
        
    def test_return_home_when_grounded(self):
        """Test return home fails when grounded."""
        drone = Drone()
        result = drone.return_home()
        assert result is False
        
    def test_battery_consumption(self):
        """Test battery is consumed during operations."""
        drone = Drone(battery_capacity=100.0)
        initial_battery = drone.battery_level
        drone.takeoff(10.0)
        drone.move_to(50.0, 50.0)
        assert drone.battery_level < initial_battery
        
    def test_telemetry(self):
        """Test telemetry data."""
        drone = Drone(name="TelemetryTest")
        drone.takeoff(15.0)
        telemetry = drone.get_telemetry()
        
        assert telemetry["name"] == "TelemetryTest"
        assert telemetry["position"]["z"] == 15.0
        assert telemetry["state"] == "hovering"
        assert "battery_level" in telemetry
        
    def test_repr(self):
        """Test string representation."""
        drone = Drone(name="ReprTest")
        repr_str = repr(drone)
        assert "ReprTest" in repr_str
        assert "grounded" in repr_str
