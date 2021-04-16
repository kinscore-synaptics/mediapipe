/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
///
/// Synap Neural Network.
///

#pragma once
#include <memory>
#include <string>
#include "mediapipe/util/synap/inc/synap/tensor.hpp"

namespace synaptics {
namespace synap {

class SynapProfilingData;
class NetworkPrivate;

/// Synap Neural Network.
class Network {
    // Implementation details
    std::unique_ptr<NetworkPrivate> d;

public:
    Network();
    ~Network();


    /// Load model.
    ///
    /// @param nbg_file            Path to a network binary graph file
    /// @param nbg_meta_file       Path to the network binary graph's metadata
    /// @return                    true if success
    bool load_model(const std::string& nbg_file, const std::string& nbg_meta_file);


    /// Load model.
    ///
    /// @param nbg_data            Network binary graph data, as from e.g. fread()
    /// @param nbg_data_size       Size in bytes of nbg_data
    /// @param nbg_meta_data       Network binary graph's metadata (JSON-formatted text)
    /// @return                    true if success
    bool load_model(const void* nbg_data, size_t nbg_data_size, const char* nbg_meta_data);


    /// Run inference.
    /// Input data to be processed are read from input tensor(s).
    /// Inference results are generated in output tensor(s).
    ///
    /// @return true if success
    bool predict();


    /// Get profiling data on last inference executed.
    ///
    /// @return profiling data
    const SynapProfilingData& get_profiling() const;


    /// Input tensors
    ///
    /// Access and iterate input tensors.
    Tensors inputs;


    /// Output tensors
    ///
    /// Access and iterate output tensors.
    Tensors outputs;
};


/// Get synap version.
///
/// @return version number
SynapVersion synap_version();

}  // namespace synap
}  // namespace synaptics
