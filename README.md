## Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data

Example code for tracing photons in time-varying heterogenous media using a visual importance map as described in the publication "Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data".
Note that each folder may have different licenses.

Please cite the article when making use of this code:

> @article{JFY16,<br>
 author       = {J{\"o}nsson, Daniel and Ynnerman, Anders},<br>
  title        = {{Correlated Photon Mapping for Interactive Global Illumination of Time-Varying Volumetric Data}},<br>
  journal      = {IEEE Transactions on Visualization and Computer Graphics (TVCG)},<br>
  number       = {1},<br>
  volume       = {23},<br>
  pages        = {901 - 910},<br>
  year         = {2017}<br>
}

#### Build
1. Download and setup [Inviwo](https://github.com/inviwo/inviwo) (tested with version Inviwo commit [989dc16e00aff2e3955281dbcf5a224ec53d2001](https://github.com/inviwo/inviwo/tree/989dc16e00aff2e3955281dbcf5a224ec53d2001)):
 - Download link: https://github.com/inviwo/inviwo/tree/989dc16e00aff2e3955281dbcf5a224ec53d2001
 - Setup instructions: https://github.com/inviwo/inviwo/wiki
 - OpenCL Required (For Nvidia: https://developer.nvidia.com/cuda-toolkit)
 
2. Download and setup boost (required by the radix sorting library)
  - http://www.boost.org/


3. Set the directory to this folder in CMake, IVW_EXTERNAL_MODULES path/to/CorrelatedPhotonMapping/modules;
- Press Configure in CMake
4. Enable the IVW_MODULE_PROGRESSIVE_PHOTONMAPPING
 - Press Configure/Generate in CMake until no errors appear.
 - Compile and run!
 - Load workspace workspaces/CorrelatedPhotonMappingSingleVolume.inv for and example. 
 - Be patient: Optimal OpenCL workgroup sizes are found for sorting the first time loading the workspace. 

#### Build system
 - The project and module configuration/generation is performed through CMake.
 - [Inviwo](https://github.com/inviwo/inviwo) interactive visualization workshop is required.
 - OpenCL Required (For Nvidia: https://developer.nvidia.com/cuda-toolkit)
 - Boost Required
 
### Licenses
Each folder can be under a different license. See license.txt in each folder. 

radixsortcl: clogs 1.5.0 is under MIT license 

rndgenmwc64x: MWC64X is under BSD license

uniformgridcl: is under MIT license

importancesamplingcl is under Creative Commons Attribution-NonCommercial 4.0 International license 

lightcl is under Creative Commons Attribution-NonCommercial 4.0 International license 

progressivephotonmapping is under Creative Commons Attribution-NonCommercial 4.0 International license 
