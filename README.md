## BRE (Bertoa Rendering Engine)

BRE is a rendering framework or engine which purpose is to have a codebase on which develop techniques related to computer graphics. Among BRE features we can include:

    - Task-based architecture for parallel draw submission.
    - Asynchronous command execution/command recording
    - An easy to read, understand and write scene format
    - Configurable number of queued frames to keep the GPU busy
    - Deferred shading

And the rendering techniques implemented at the moment are

    - Color Mapping
    - Texture Mapping
    - Normal Mapping
    - Height Mapping
    - Color Normal Mapping
    - Color Height Mapping
    - Skybox Mapping
    - Diffuse Irradiance Environment Mapping
    - Specular Pre-Convolved Environment Mapping
    - Tone Mapping
    - Screen Space Ambient Occlusion
    - Gamma Correction


## Repository structure
The directory structure is:

	/BRE		Source code
	/external	Third-party libraries
	/doc		Documentation (doxygen) - Open index file.
	

## Examples and Documentation

In the Visual Studio solution, you can check scene files in BRE/Executable/resources/scenes. I use YAML format for scenes.
You can open /doc/index.html file to read the Doxygen documentation
You can check the following blog entry where I constantly add new articles about BRE (architecture, techniques, classes, etc). It is https://nbertoa.wordpress.com/bre/


## Blog

I write a blog where I explain its design, and how I applied/learned the techniques I use.
The blog link is https://nbertoa.wordpress.com/
