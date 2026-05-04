

https://github.com/user-attachments/assets/495704ef-412c-4019-82e0-d63d0aec17e0

# Character Animator - 3D Character Animation Project

A CMake-based OpenGL project showcasing an animated 3D character with real-time movement and animation controls.

## Features

- **3D Character Model**: Uses Assimp to load animated character models (supports .dae, .fbx, .gltf, etc.)
- **Skeletal Animation**: Implements bone-based animation with smooth interpolation
- **Real-time Movement**: WASD controls for movement, arrow keys for rotation, UP/DOWN for height
- **Animation Switching**: Press number keys (1-9) to switch between available animations
- **Optimized Rendering**: GPU-accelerated rendering with OpenGL 3.3 Core Profile

## Requirements

- CMake 3.20 or later
- C++17 compiler
- Windows, macOS, or Linux
- GPU with OpenGL 3.3+ support

### External Dependencies

- **GLFW3**: Window management and input handling
- **GLM**: Linear algebra math library
- **Assimp**: 3D model loading library
- **GLAD**: OpenGL loader
- **stb_image**: Image loading library

Dependencies are automatically fetched by CMake (except Assimp on Linux, which uses system installation).

## Building

### Windows (Visual Studio 2019+)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### Windows (MinGW)

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --config Release
```

### macOS

```bash
mkdir build
cd build
cmake .. -G "Unix Makefiles"
cmake --build . --config Release
```

### Linux

```bash
# Install Assimp first
sudo apt-get install libassimp-dev

mkdir build
cd build
cmake .. -G "Unix Makefiles"
cmake --build . --config Release
```

## Running

After building, run the executable:

```bash
./build/CharacterAnimator  # Linux/macOS
./build/Release/CharacterAnimator.exe  # Windows
```

## Controls

### Movement
- **W**: Move forward
- **S**: Move backward
- **A**: Strafe left
- **D**: Strafe right
- **UP Arrow**: Move up
- **DOWN Arrow**: Move down
- **LEFT Arrow**: Rotate counter-clockwise
- **RIGHT Arrow**: Rotate clockwise

### Animation
- **1-9**: Switch between available animations

## Project Structure

```
character_animator/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Entry point and main loop
│   ├── math.h/cpp          # Math utilities (Vec3, Mat4, quaternions)
│   ├── assets.h/cpp        # Model loading and animation handling
│   ├── graphics.h/cpp      # OpenGL rendering utilities
│   └── stb_image.h         # Image loading (single-file header)
└── shaders/
    ├── character.vs        # Vertex shader with skeletal animation
    └── character.fs        # Fragment shader
```

## Asset Paths

The project looks for resources in these locations (in order):
1. `./resources/` (current working directory)
2. `../resources/` (parent directory)
3. `../project/resources/` (sibling project directory)

Place your character models and animations in the `resources/object/` directory.

## Supported Model Formats

- COLLADA (.dae)
- FBX (.fbx)
- glTF (.gltf, .glb)
- OBJ (.obj, limited animation support)
- And many others supported by Assimp

## Character Models

The project comes with models from the `project/resources/object/mixamo/` directory:
- Standing Walk Forward.dae
- Unarmed Run Forward.dae
- Great Sword Idle.dae
- Vanguard By T. Choonyung/ (character with textures)

## Implementation Notes

### Animation System
- Uses Assimp's built-in animation data
- Supports multiple animation clips per model
- Smooth interpolation between keyframes using linear interpolation for position/scale and SLERP for rotation
- FPS-independent animation updates

### Rendering
- Uses OpenGL 3.3 Core Profile for compatibility
- Shader-based rendering with vertex/fragment shaders
- Support for textured meshes with transparency
- Depth testing enabled for proper 3D rendering

### Math Library
- Custom Vec3 and Mat4 implementations for basic math
- Matrix multiplication and transformation operations
- Quaternion math for smooth rotations
- Inverse trigonometry for character orientation

## Troubleshooting

### Model not loading
- Verify the resource path is correct
- Check that the model file format is supported
- Ensure textures are in the correct location relative to the model

### No textures showing
- Verify texture files exist in the model's directory
- Check OpenGL error messages in console output
- Ensure texture file formats are supported (PNG, JPG, etc.)

### Performance issues
- Reduce animation update frequency if needed
- Consider LOD (Level of Detail) for complex models
- Profile with your GPU's debugging tools

## Future Enhancements

- IK (Inverse Kinematics) for more natural animations
- Blending between animations
- Physics-based character movement
- Multiple characters with collision detection
- Advanced lighting and shadows
- Animation recording and playback

## License

This project uses libraries with their respective licenses:
- GLFW: zlib/libpng
- GLM: Happy Bunny License
- Assimp: BSD-3-Clause
- GLAD: Generated code (public domain)
- stb_image: Public domain
