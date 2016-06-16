/*********************************************************************************
 *
 * Copyright (c) 2016, Daniel Jönsson
 * All rights reserved.
 * 
 * This work is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.
 * http://creativecommons.org/licenses/by-nc/4.0/
 * 
 * You are free to:
 * 
 * Share — copy and redistribute the material in any medium or format
 * Adapt — remix, transform, and build upon the material
 * The licensor cannot revoke these freedoms as long as you follow the license terms.
 * Under the following terms:
 * 
 * Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
 * NonCommercial — You may not use the material for commercial purposes.
 * No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/lightcl/lightclmodule.h>
#include <modules/opencl/kernelmanager.h>
#include <modules/lightcl/processors/directionallightsamplerclprocessor.h>

namespace inviwo {

LightCLModule::LightCLModule(InviwoApplication* app) : InviwoModule(app, "LightCL") {
    
    // Add a directory to the search path of the Shadermanager
    //ShaderManager::getPtr()->addShaderSearchPath(PathType::Modules, "lightcl/glsl");
    
    // Register objects that can be shared with the rest of inviwo here:
    
    // Processors
    registerProcessor<DirectionalLightSamplerCLProcessor>();
    
    // Properties
    // registerProperty<LightCLProperty>();
    
    // Readers and writes
    // registerDataReader<new LightCLReader()>();
    // registerDataWriter(new LightCLWriter());
    
    // Data converters
    // registerRepresentationConverter(new LightCLDisk2RAMConverter());

    // Ports
    registerPort< LightSamplesInport >("org.inviwo.LightSamplesInport");
    registerPort< LightSamplesOutport >("org.inviwo.LightSamplesOutport");

    registerPort< MultiDataInport<LightSamples> >("org.inviwo.LightSamplesMultiInport");

    // PropertyWidgets
    // registerPropertyWidget(LightCLPropertyWidgetQt, LightCLProperty, "Default");
    
    // Dialogs
    // registerDialog("lightcl", LightCLDialogQt);
    
    // Other varius things
    // registerCapabilities(Capabilities* info);
    // registerData(Data* data);
    // registerDataRepresentation(DataRepresentation* dataRepresentation);
    // registerSettings(new SystemSettings());
    // registerMetaData(MetaData* meta);   
    // registerPortInspector(PortInspector* portInspector);
    // registerProcessorWidget(std::string processorClassName, ProcessorWidget* processorWidget);
    // registerDrawer(GeometryDrawer* renderer);
    // registerResource(Resource* resource);    
    OpenCL::getPtr()->addCommonIncludeDirectory(getPath(ModulePath::CL));
}

} // namespace
