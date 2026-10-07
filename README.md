## BRE (Bertoa Rendering Engine)

> **Historical project — 2017.** BRE is preserved as a record of my earlier DirectX 12 and real-time graphics R&D. It is not presented as current production work.

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
You can open /doc/index.html file to read the Doxygen documentation.
The portfolio page collects the BRE architecture series, context, source links, and videos: https://nbertoa.com/bre/


## Blog

My current portfolio and R&D archive are available at https://nbertoa.com/

## License

Our license is based on the modified, 3-clause BSD-License.

An informal summary is: do whatever you want, but include BRE's license text with your product - and don't sue us if our code doesn't work. Note that, unlike LGPLed code, you may link statically to BRE. For the legal details, see the LICENSE file.

