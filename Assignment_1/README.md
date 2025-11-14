# Assignment 1: 2D Animation

## Description
This assignment demonstrates 2D animation using transformation matrices and mathematical functions (sine/cosine) to create smooth, dynamic animations.

## Features
- **Multiple animated objects** with different transformation patterns:
  1. **Orbiting object**: Rotates around the center while spinning and pulsing
  2. **Counter-orbiting object**: Moves in opposite direction with different rotation speed
  3. **Sine wave motion**: Moves vertically along a sine wave path
  4. **Figure-8 pattern**: Follows a Lissajous curve (figure-8 pattern)
  5. **Center breathing object**: Pulsates at the center with slow rotation

- **Mathematical animations** using:
  - `sin()` and `cos()` functions for smooth motion
  - Transformation matrices (translate, rotate, scale)
  - Time-based animations using `glfwGetTime()`

## Building
1. Configure CMake:
   ```bash
   cmake -B build
   ```

2. Build the project:
   ```bash
   cmake --build build
   ```

3. Run the executable:
   ```bash
   ./build/Assignment_1/Assignment_1
   ```

## Adding Your Image
Place your `image.png` file in the `Assignment_1/resources/textures/` directory. The program will automatically load it as the primary texture. If the image is not found, it will fall back to the default container texture.

## Controls
- **ESC**: Exit the application

## Technical Details
- Uses GLFW for window management
- GLAD for OpenGL function loading
- GLM for matrix transformations
- STB Image for texture loading
- Vertex and fragment shaders for rendering

## Animation Techniques Demonstrated
1. **Circular motion**: Using `cos(time)` and `sin(time)` for x and y coordinates
2. **Rotation**: Using `glm::rotate()` with time-based angles
3. **Scaling**: Using `sin(time)` for pulsing effects
4. **Complex paths**: Lissajous curves using different frequencies for x and y
5. **Combined transformations**: Multiple transformations applied in sequence

