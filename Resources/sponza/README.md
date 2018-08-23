# Sponza

## link
http://www.crytek.com/cryengine/cryengine3/downloads

## Usage

Download sponza.obj, sponza.mtl, textures.

Move textures under the same directory where obj and mtl files locate.

Load "Resources/sponza/sponza.obj" with ASSIMP.

## Tips For Different Platforms

Edit paths to textures in sponza.mtl.

If you use it on Windows, path separator is '\';
On MacOS and Linux, path separator is '/'

## Tips For OpenGL

Translate matrix can be set to:

```
Camera camera(glm::vec3(0.0f, 0.0f, 30.0f));
...
glm::mat4 objectMatrix;
objectMatrix = glm::translate(objectMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
objectMatrix = glm::scale(objectMatrix, glm::vec3(0.02f, 0.02f, 0.02f));
```