/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2016 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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

#include "radixsortcl/radixsortclmodule.h"
#include "src/clhpp11.h"
#include "radixsortcl/processors/radixsortcl.h"

#include <inviwo/core/io/textfilereader.h>
#include <opencl/inviwoopencl.h>
#include <warn/push>
#include <warn/ignore/conversion>
#include "src/utils.h"
#include <warn/pop>


 // add by myself
#ifndef  _My_env
#define	 _My_env
#include "inviwo/core/util/filesystem.h"
#include "inviwo/core/io/datareaderfactory.h"
#include "inviwo/core/common/inviwoapplication.h"
#include "inviwo/core/io/datawriterfactory.h"
#include "inviwo/core/properties/buttonproperty.h"
#endif // ! _My_env

namespace inviwo {

RadixSortCLModule::RadixSortCLModule(InviwoApplication* app) : InviwoModule(app, "RadixSortCL") {
    registerProcessor<RadixSortCL>();

    OpenCL::getPtr()->addCommonIncludeDirectory(getPath() + "/kernels");
    for (const auto& elem : OpenCL::getPtr()->getCommonIncludeDirectories()) {
        std::string pathToKernels = elem;
        try {
            if (filesystem::fileExists(pathToKernels + "/radixsort.cl"))
                addSourceToClogs(pathToKernels + "/", "radixsort.cl", "431a3a83882a2497d57d49faafc95f3caceaeca4a42aca9623b3aae7dc6cf4ee");
            if (filesystem::fileExists(pathToKernels + "/reduce.cl"))
                addSourceToClogs(pathToKernels + "/", "reduce.cl", "52c419ceb4263cc36ca2f9297b10fe98c21173aabde3c02609358d0834f51f91");
            if (filesystem::fileExists(pathToKernels + "/scan.cl"))
                addSourceToClogs(pathToKernels + "/", "scan.cl", "dbf441df48411f177a18b899f9472737a4711c843b4c25907b756274a911a437");
        } catch (std::ifstream::failure& ex) {
            LogError(ex.what());
        }
    }
}
void RadixSortCLModule::addSourceToClogs(const std::string& path, const std::string& fileName, const std::string& hash) {
    TextFileReader fileReader(path + fileName);
    std::string prog;
    try {
        prog = fileReader.read();
        clogs::detail::getSourceMap()[fileName] = clogs::detail::Source(prog, hash);
    } catch (std::ifstream::failure& ex) {
        throw ex;
    }

}


RadixSortCLModule::~RadixSortCLModule()
{
}

} // namespace
