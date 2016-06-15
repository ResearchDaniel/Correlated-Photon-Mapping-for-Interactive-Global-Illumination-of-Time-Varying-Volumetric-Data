## Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data

Example code for tracing photons in time-varying heterogenous media using a visual importance map as described in the publication "Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data".
Note that each folder may have different licenses.

Please cite the article when making use of this code:

> @article{JFY16,<br>
 author       = {J{\"o}nsson, Daniel and Ynnerman, Anders},<br>
  title        = {{Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data}},<br>
  journal      = {IEEE Transactions on Visualization and Computer Graphics (TVCG)},<br>
  number       = {X},<br>
  volume       = {X},<br>
  pages        = {XX - XX},<br>
  year         = {In press 2016}<br>
}

#### Build
1. Download and setup Inviwo:
 - Download link: https://github.com/inviwo/inviwo
 - Setup instructions: https://github.com/inviwo/inviwo/wiki
 - OpenCL Required (For Nvidia: https://developer.nvidia.com/cuda-toolkit)
 
2. Download and setup boost (required by the radix sorting library)
  - http://www.boost.org/


3. Set the directory to this folder in CMake, IVW_EXTERNAL_MODULES path/to/CorrelatedPhotonMapping/modules;
- Press Configure in CMake
4. Enable the IVW_MODULE_PROGRESSIVE_PHOTONMAPPING
 - Press Configure/Generate in CMake until no errors appear.
 - Compile and run!
 - Load workspace workspaces/CorrelatedPhotonMapping.inv for and example. 
 - Be patient: Optimal OpenCL workgroup sizes are found for sorting the first time loading the workspace. 

#### Build system
 - The project and module configuration/generation is performed through CMake (>= 2.8.11).
 - Inviwo has been compiled in Visual Studio (>= 2015), XCode (>= 5), KDevelop (>= 4), Make.
 - C++11 Required
 - OpenCL Required (For Nvidia: https://developer.nvidia.com/cuda-toolkit)
 - Boost Required
 
### Licenses
Each folder can be under a different license. See license.txt in each folder. 

clogs 1.5.0 is under MIT license

MWC64X is under BSD license
