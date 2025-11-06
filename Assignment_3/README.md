# Assignment 3: Simple 3D Game

## Overview
This is a simple 3D game implementation with the following features:
1. **Loading 3D Models from Files**: Load player models, scenes, items, and enemies from OBJ files
2. **Player Control**: Control the player's model with keyboard input
3. **Camera Following**: Camera that automatically follows the player
4. **Collision Detection**: AABB collision detection between player-scene, player-items, etc.

## Media

### Screenshot
![Game Screenshot](resources/Car.png)

### Demo Video
[Watch Demo Video](resources/Car.mp4)

<video src="resources/Car.mp4" controls width="100%">
  Your browser does not support the video tag.
</video>

## Features

### 3D Model Loading
- **OBJ Loader**: Generic OBJ file loader that supports:
  - Position (v)
  - Texture coordinates (vt)
  - Normals (vn)
  - Faces (f) - triangles and quads
- **Automatic Fallback**: If model file not found, uses simple cube geometry
- **Texture Support**: Load and apply textures to models
- **AABB Calculation**: Automatically calculates bounding boxes for collision detection

### Player Control
- **W/A/S/D**: Move player in 3D space
- **Collision Prevention**: Player cannot move through obstacles

### Camera Following
- Camera automatically follows the player object
- Smooth offset positioning (behind and above the player)
- Always looks at the player position

### Collision Detection
- **AABB (Axis-Aligned Bounding Box)** collision detection
- Prevents player from moving through obstacles
- Detects collisions between player and scene/items/enemies

