## BRE (Bertoa Rendering Engine)

This engine is the result of an intent to learn DirectX 12 and apply my knowledge about computer graphics. It is intended for learning purposes only, I do not plan to make a game or a commercial engine. 

## Features

DirectX 12:
- Task based architecture for parallel draw submission. Command lists are recorded in parallel for Geometry Pass and Lighting Pass.
- Asynchronous command execution/command recording.
- Configurable number of queued frames to keep the GPU busy.

Rendering:
- Deferred Shading
- Color Mapping
- Texture Mapping
- Normal Mapping
- Height Mapping

Lighting:
- Physically Based Shading (PBR) based on smoothness/metalness
- Image Based Lighting (IBL) based on diffuse irradiance environment cube map and specular pre-convolved environment cube map
- Punctual lights
 
Postprocessing:
- Tone Mapping

## Examples

In the Visual Studio solution, there is a project called ExampleScenes where you can see a lot of different techniques (color mapping, normal mapping, height mapping, cube mapping, etc)

## Blog

I write a blog where I explain its design, and how I applied/learned the techniques I use.
The blog link is https://nbertoa.wordpress.com/
