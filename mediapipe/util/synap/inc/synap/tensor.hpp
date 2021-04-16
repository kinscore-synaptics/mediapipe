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
/// Synap data tensor.
///

#pragma once

#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <iostream>

#include "mediapipe/util/synap/inc/synap/types.hpp"
#include "mediapipe/util/synap/inc/synap/buffer.hpp"

namespace synaptics {
namespace synap {

class NetworkPrivate;
class NbgTensorAttributes;

/// Synap data tensor.
class Tensor {
public:
    /// Dimensions
    typedef std::vector<int32_t> Shape;
    
    /// In/out type
    enum class Type {
        none,
        in,
        out
    };

    /// Information and attributes
    struct Info {
        std::string name;
        Type type{};
        Layout layout{};
        Shape shape;
        SynapType data_type{};
        size_t size() const;
        size_t item_count() const;
    };

    /// Constructor.
    /// User can only access tensors created by the network itself.
    Tensor(NetworkPrivate* np, int32_t ix, const Info& info, const NbgTensorAttributes* attr) :
        _np{np}, _index{ix}, _info{info}, _attr(attr)
    {
    }

    /// @return tensor name
    const std::string& name() const { return _info.name; }
    
    /// @return tensor shape
    const Shape& shape() const { return _info.shape; }

    /// @return tensor layout
    const Layout& layout() const { return _info.layout; }

    /// @return tensor size in bytes
    size_t size() const { return _info.size(); }

    /// @return number of data items in the tensor
    size_t item_count() const { return _info.item_count(); }

    /// @return tensor data type
    const SynapType data_type() const { return _info.data_type; }
    
    /// Copy data in tensor buffer.
    /// @param data: data to be copied in tensor. Data size must match tensor size.
    /// @return true if success
    bool assign(const std::vector<uint8_t>& data);

    /// @return pointer to float[item_count()] array representing tensor content converted to float
    ///         (nullptr if tensor has no data)
    /// The pointer must NOT be released and the content will be valid until the next inference.
    const float* as_float() const;


    /// Tensor raw data pointer if any.
    /// @return pointer to the raw data inside the data buffer, nullptr if none.
    const void* data() const;    
    void* data();

    /// Get current Buffer if any.
    /// This will be the default tensor Buffer unless another Buffer has been set with set_buffer()
    /// @return current data buffer, or nullptr if none
    Buffer* buffer();

    /// Set current data buffer.
    /// @param buffer: buffer to be used for this tensor.
    ///                The buffer size must match the tensor size.
    /// @return true if success
    bool set_buffer(Buffer* buffer);

    
private:

    /// Associated network
    NetworkPrivate* _np{};
    
    /// Tensor index (@todo: to be removed if possible)
    int32_t _index;

    /// Tensor info
    Info _info{};
    
    /// Tensor attributes
    const NbgTensorAttributes* _attr{};
    
    /// Default buffer (used if no external buffer assigned to the tensor)
    Buffer _default_buffer{};

    /// Current data buffer if any
    Buffer* _buffer{&_default_buffer};
    
    /// Current data buffer set to the network if any.
    /// Always equivalent to _buffer except at the beginning when it is null.
    Buffer* _set_buffer{};
    
    /// Contains dequantized data if dequantization not done by the network itself
    mutable std::vector<float> _dequantized_data;
};


/// Tensor collection
class Tensors {
public:
    /// Construct from a vector of tensors
    Tensors(std::vector<Tensor>& tensors) : _tensors{tensors} {}

    /// Access/iteration methods
    size_t size() const { return _tensors.size(); }
    const Tensor& operator[](size_t index) const;
    Tensor& operator[](size_t index) {    
        return const_cast<Tensor &>(static_cast<const Tensors &>(*this)[index]);
    }
    std::vector<Tensor>::iterator begin() { return _tensors.begin(); }
    std::vector<Tensor>::iterator end() { return _tensors.end(); }

private:
    std::vector<Tensor>& _tensors;
};


inline std::ostream& operator<<(std::ostream& os, const Tensor::Shape& v)
{
    os << '[';
    for (int i = 0; i < v.size(); ++i) {
        os << (i ? ", " : "") << v[i];
    }
    os << ']';
    return os;
}

}
}
