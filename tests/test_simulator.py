"""Unit tests for the FlightSimulator class."""

import pytest
from src.drone import Drone, DroneState, Position
from src.simulator import FlightSimulator


class TestFlightSimulator:
    """Tests for the FlightSimulator class."""
    
    def test_initialization(self):
        """Test simulator initialization."""
        sim = FlightSimulator(boundary_x=500, boundary_y=500)
        assert sim.boundary_x == 500
        assert sim.boundary_y == 500
        assert len(sim.drones) == 0
        
    def test_add_drone(self):
        """Test adding a drone."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        assert len(sim.drones) == 1
        assert sim.drones[0].name == "Test-1"
        
    def test_remove_drone(self):
        """Test removing a drone."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        removed = sim.remove_drone("Test-1")
        assert removed is not None
        assert removed.name == "Test-1"
        assert len(sim.drones) == 0
        
    def test_remove_nonexistent_drone(self):
        """Test removing a drone that doesn't exist."""
        sim = FlightSimulator()
        result = sim.remove_drone("NonExistent")
        assert result is None
        
    def test_get_drone(self):
        """Test getting a drone by name."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        found = sim.get_drone("Test-1")
        assert found is not None
        assert found.name == "Test-1"
        
    def test_get_nonexistent_drone(self):
        """Test getting a drone that doesn't exist."""
        sim = FlightSimulator()
        found = sim.get_drone("NonExistent")
        assert found is None
        
    def test_get_all_telemetry(self):
        """Test getting telemetry from all drones."""
        sim = FlightSimulator()
        sim.add_drone(Drone(name="Test-1"))
        sim.add_drone(Drone(name="Test-2"))
        telemetry = sim.get_all_telemetry()
        assert len(telemetry) == 2
        
    def test_is_within_bounds(self):
        """Test boundary checking."""
        sim = FlightSimulator(boundary_x=100, boundary_y=100)
        assert sim.is_within_bounds(Position(50, 50, 10)) is True
        assert sim.is_within_bounds(Position(150, 50, 10)) is False
        assert sim.is_within_bounds(Position(-10, 50, 10)) is False
        
    def test_check_collisions_no_collision(self):
        """Test collision check with no collisions."""
        sim = FlightSimulator()
        drone1 = Drone(name="Test-1")
        drone2 = Drone(name="Test-2")
        sim.add_drone(drone1)
        sim.add_drone(drone2)
        
        drone1.takeoff(10)
        drone2.takeoff(10)
        drone1.move_to(0, 0, 10)
        drone2.move_to(100, 100, 10)
        
        collisions = sim.check_collisions()
        assert len(collisions) == 0
        
    def test_check_collisions_with_collision(self):
        """Test collision check with potential collision."""
        sim = FlightSimulator()
        drone1 = Drone(name="Test-1")
        drone2 = Drone(name="Test-2")
        sim.add_drone(drone1)
        sim.add_drone(drone2)
        
        drone1.takeoff(10)
        drone2.takeoff(10)
        drone1.move_to(0, 0, 10)
        drone2.move_to(2, 2, 10)  # Within 5m default
        
        collisions = sim.check_collisions()
        assert len(collisions) == 1
        
    def test_run_mission(self):
        """Test running a mission."""
        sim = FlightSimulator(boundary_x=500, boundary_y=500)
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        drone.takeoff(10)
        
        waypoints = [(50, 50, 20), (100, 100, 25)]
        result = sim.run_mission("Test-1", waypoints)
        assert result is True
        assert drone.position.x == 100
        assert drone.position.y == 100
        
    def test_run_mission_nonexistent_drone(self):
        """Test running mission for nonexistent drone."""
        sim = FlightSimulator()
        result = sim.run_mission("NonExistent", [(50, 50, 10)])
        assert result is False
        
    def test_run_mission_out_of_bounds(self):
        """Test mission fails if waypoint out of bounds."""
        sim = FlightSimulator(boundary_x=100, boundary_y=100)
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        drone.takeoff(10)
        
        waypoints = [(200, 200, 20)]  # Out of bounds
        result = sim.run_mission("Test-1", waypoints)
        assert result is False
        
    def test_emergency_land_all(self):
        """Test emergency landing."""
        sim = FlightSimulator()
        drone1 = Drone(name="Test-1")
        drone2 = Drone(name="Test-2")
        sim.add_drone(drone1)
        sim.add_drone(drone2)
        
        drone1.takeoff(10)
        drone2.takeoff(20)
        
        sim.emergency_land_all()
        
        assert drone1.state == DroneState.GROUNDED
        assert drone2.state == DroneState.GROUNDED
        
    def test_return_all_home(self):
        """Test return all drones home."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        
        drone.takeoff(10)
        drone.set_home_position()
        drone.move_to(50, 50, 15)
        
        sim.return_all_home()
        
        assert drone.state == DroneState.GROUNDED
        assert drone.position.x == 0.0
        assert drone.position.y == 0.0
        
    def test_flight_log(self):
        """Test flight logging."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        
        log = sim.get_flight_log()
        assert len(log) > 0
        assert log[0]["event"] == "added"
        
    def test_clear_flight_log(self):
        """Test clearing flight log."""
        sim = FlightSimulator()
        sim.add_drone(Drone(name="Test-1"))
        sim.clear_flight_log()
        assert len(sim.get_flight_log()) == 0
        
    def test_status_report(self):
        """Test status report generation."""
        sim = FlightSimulator()
        drone = Drone(name="Test-1")
        sim.add_drone(drone)
        drone.takeoff(10)
        
        report = sim.get_status_report()
        assert "Test-1" in report
        assert "hovering" in report
