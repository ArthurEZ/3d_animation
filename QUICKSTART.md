# Quick Start Guide

## Project Setup

Your new character animation project has been created in the `character_animator` folder with the following structure:

```
character_animator/
├── CMakeLists.txt          # Build configuration
├── README.md               # Full documentation
├── QUICKSTART.md           # This file
├── src/
│   ├── main.cpp            # Main application with animation loop
│   ├── math.h/cpp          # Math utilities
│   ├── assets.h/cpp        # Model and animation loading
│   └── graphics.h/cpp      # OpenGL rendering
└── shaders/
    ├── character.vs        # Vertex shader
    └── character.fs        # Fragment shader
```

## Quick Build Instructions

### Windows (Recommended: Visual Studio)

1. Open PowerShell or Command Prompt
2. Navigate to the character_animator folder:
   ```bash
   cd f:\thanagorn\work\cedt\02_002\select_68\character_animator
   ```

3. Create build directory and configure with CMake:
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019" -A x64
   ```

4. Build the project:
   ```bash
   cmake --build . --config Release
   ```

5. Run the application:
   ```bash
   .\Release\CharacterAnimator.exe
   ```

### macOS / Linux

```bash
cd f:/thanagorn/work/cedt/02_002/select_68/character_animator
mkdir build
cd build
cmake ..
cmake --build . --config Release
./CharacterAnimator
```

## Runtime Controls

Once the application is running:

### Camera & Movement
- **W/A/S/D**: Move forward/left/backward/right
- **UP/DOWN arrows**: Move up/down
- **LEFT/RIGHT arrows**: Rotate left/right

### Animation Control
- **1-9**: Switch between loaded animations
- **Animations available**: Press any of these to see loaded animation names in console

## Features

✅ **Full 3D character movement** with WASD controls  
✅ **Multiple animations** loaded from the character model  
✅ **Real-time skeletal animation** with smooth interpolation  
✅ **GPU-accelerated rendering** with OpenGL 3.3  
✅ **Textured character model** from your project folder  

## What's Included

### Core Components

1. **Model Loading** (`assets.cpp`)
   - Uses Assimp to load character models (.dae, .fbx, .gltf, etc.)
   - Extracts skeletal data and animation clips
   - Manages bone transforms and vertex weights

2. **Animation System** (`assets.cpp`)
   - Updates bone transforms based on animation time
   - Supports smooth keyframe interpolation
   - Handles multiple animation clips

3. **Rendering** (`graphics.cpp`, shaders)
   - OpenGL 3.3 Core Profile rendering
   - Vertex/fragment shaders with skeletal animation support
   - Texture support with alpha blending

4. **Input Handling** (`main.cpp`)
   - WASD movement with smooth physics
   - Character rotation with arrow keys
   - Animation switching with number keys

## Character Models Used

The project loads character models from `../project/resources/object/mixamo/`:
- **Standing Walk Forward.dae** - Walking animation
- **Unarmed Run Forward.dae** - Running animation  
- **Great Sword Idle.dae** - Combat idle pose
- **Vanguard By T. Choonyung/** - Full textured character

## Common Issues & Solutions

### Build fails with "glad.h not found"
- The project references GLAD from the parent `project` folder
- Make sure you haven't deleted or moved the `project` folder

### Model doesn't appear or seems invisible
- Check the resource path in console output
- Verify model files exist in `../project/resources/object/mixamo/`
- The model may be very large/small - check console output

### No textures showing
- Texture loading is attempted but optional
- Check console for texture loading messages
- Model may render without textures using default color

### Animations not switching
- Press 1-9 to switch
- Check console to see available animation names
- Not all numbers may have animations if model has fewer clips

## Code Highlights

### Loading a Character
```cpp
AnimatedCharacter character;
std::string error;
if (!load_animated_character(model_path, character, error)) {
    std::cerr << "Failed: " << error << std::endl;
}
```

### Updating Animation
```cpp
update_animated_character(character, animation_index, time_seconds);
```

### Rendering Character
```cpp
render_character(character, world_matrix, shader_program);
```

## Next Steps

1. **Run the application** and verify the character appears
2. **Test animations** with number keys (1-9)
3. **Explore the code** in `src/` to understand the structure
4. **Customize movement** - Edit `process_input()` in main.cpp
5. **Add features** like:
   - Animation blending
   - Multiple characters
   - Physics-based movement
   - IK animations

## Technical Details

### Skeletal Animation Pipeline
1. Load model with Assimp
2. Extract skeleton hierarchy and animation clips
3. Each frame: sample animation keyframes
4. Update bone transforms with SLERP interpolation
5. Pass bone transforms to vertex shader
6. GPU applies bone transformations to vertices

### 3D Transformation
- World matrix: Position × Rotation × Scale
- View matrix: Camera position + target + up
- Projection matrix: Perspective 45° FOV
- All combined in vertex shader

## Performance Considerations

- Loads one character model at startup
- CPU-based animation updates (could be GPU-optimized)
- ~60 FPS target with vsync enabled
- Scales down model by 0.01x in rendering

## Resources for Learning

- **Assimp Documentation**: https://assimp-docs.readthedocs.io/
- **OpenGL Tutorials**: https://learnopengl.com/
- **GLM Math Library**: https://glm.g-truc.net/
- **GLFW Window Library**: https://www.glfw.org/

---

**Enjoy your character animator project!** 🎮✨
