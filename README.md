# Drone Project

A Python-based drone flight simulation framework for learning and experimentation.

## Overview

This project provides a simple yet comprehensive drone simulation environment that includes:

- **Drone Class**: Core drone model with realistic flight operations
- **Flight Simulator**: Environment for managing multiple drones
- **Telemetry System**: Real-time status monitoring
- **Mission Planning**: Waypoint-based flight path execution

## Features

- Take off and landing operations
- 3D movement and positioning
- Battery management
- Collision detection
- Home position and return-to-home functionality
- Flight logging
- Multi-drone support

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/samuel-sjoden/drone-project.git
   cd drone-project
   ```

2. Create a virtual environment (recommended):
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

3. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

## Usage

### Running the Demo

```bash
python main.py
```

This will run a demonstration showing:
- Creating and managing multiple drones
- Takeoff and landing sequences
- Waypoint navigation
- Telemetry monitoring
- Collision detection

### Using the Drone Class

```python
from src.drone import Drone

# Create a drone
drone = Drone(name="MyDrone", max_altitude=100, max_speed=15)

# Take off
drone.takeoff(target_altitude=20)

# Move to a position
drone.move_to(x=50, y=50, z=25)

# Get telemetry
telemetry = drone.get_telemetry()
print(telemetry)

# Land
drone.land()
```

### Using the Flight Simulator

```python
from src.drone import Drone
from src.simulator import FlightSimulator

# Create simulator
sim = FlightSimulator(boundary_x=500, boundary_y=500)

# Add drones
drone = Drone(name="Alpha-1")
sim.add_drone(drone)

# Run a mission
waypoints = [(50, 50, 20), (100, 100, 25), (150, 50, 20)]
drone.takeoff(15)
sim.run_mission("Alpha-1", waypoints)

# Get status
print(sim.get_status_report())
```

## Running Tests

```bash
pytest tests/ -v
```

With coverage report:
```bash
pytest tests/ -v --cov=src --cov-report=term-missing
```

## Project Structure

```
drone-project/
├── src/
│   ├── __init__.py
│   ├── drone.py        # Core Drone class
│   └── simulator.py    # Flight simulator
├── tests/
│   ├── __init__.py
│   ├── test_drone.py      # Drone unit tests
│   └── test_simulator.py  # Simulator unit tests
├── main.py             # Demo entry point
├── requirements.txt    # Dependencies
└── README.md          # This file
```

## Drone States

The drone can be in one of the following states:

| State | Description |
|-------|-------------|
| GROUNDED | On the ground, motors off |
| TAKING_OFF | In the process of ascending |
| HOVERING | Stationary in the air |
| FLYING | Moving to a destination |
| LANDING | In the process of descending |

## API Reference

### Drone Class

| Method | Description |
|--------|-------------|
| `takeoff(altitude)` | Ascend to specified altitude |
| `land()` | Descend and land |
| `move_to(x, y, z)` | Move to coordinates |
| `hover()` | Stop and maintain position |
| `return_home()` | Return to home position |
| `get_telemetry()` | Get current status data |

### FlightSimulator Class

| Method | Description |
|--------|-------------|
| `add_drone(drone)` | Add a drone to simulation |
| `remove_drone(name)` | Remove a drone by name |
| `run_mission(name, waypoints)` | Execute waypoint mission |
| `check_collisions()` | Check for collision risks |
| `emergency_land_all()` | Land all drones immediately |
| `get_status_report()` | Get formatted status report |

## License

This project is for educational purposes.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.
