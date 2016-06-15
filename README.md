## Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data

Example code for tracing photons in time-varying heterogenous media using a visual importance map as described in the publication "Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data".
Note that each folder may have different licenses.

Please cite the article when making use of this code:

@article{JFY16,
  author       = {J{\"o}nsson, Daniel and Ynnerman, Anders},
  title        = {{Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data}},
  journal      = {IEEE Transactions on Visualization and Computer Graphics (TVCG)},
  number       = {X},
  volume       = {X},
  pages        = {XX - XX},
  year         = {In press 2016}
}

#### Build
Download and setup Inviwo:
Download link: https://github.com/inviwo/inviwo
Setup instructions: https://github.com/inviwo/inviwo/wiki
Download and setup boost (required by the radix sorting library): http://www.boost.org/


Set the directory to this folder in CMake, IVW_EXTERNAL_MODULES 
path/to/CorrelatedPhotonMapping/modules;

Enable the IVW_MODULE_PROGRESSIVE_PHOTONMAPPING
 

#### Build system
 - The project and module configuration/generation is performed through CMake (>= 2.8.11).
 - Inviwo has been compiled in Visual Studio (>= 2015), XCode (>= 5), KDevelop (>= 4), Make.
 - C++11 Required
 - OpenCL Required (For Nvidia: https://developer.nvidia.com/cuda-toolkit)
 - Boost Required
 
### Licenses
clogs 1.5.0 is under MIT license
MWC64X is under BSD license