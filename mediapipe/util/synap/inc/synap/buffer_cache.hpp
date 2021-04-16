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
/// Synap Buffer cache
///

#pragma once

#include "mediapipe/util/synap/inc/synap/buffer.hpp"
#include <map>

namespace synaptics {
namespace synap {

/// Maintains a set of Buffers.
/// This is nothing more than a small wrapper around a std::map but makes the use more explicit.
/// 
/// Example:
/// BufferCache<AMP_BD_HANDLE> buffers;
/// ...
/// AMP_BD_HANDLE bdh = ..;
/// Buffer* b = buffers.get(bdh);
/// if (!b) b = buffers.add(bdh,  get_bd_data(bdh), get_bd_size(bdh));
/// ...
template<typename Id>
class BufferCache {
public:
    typedef std::map<Id, Buffer> Map;

    /// Create Buffer set.
    /// @param allow_cpu_access: if true buffers will be created with CPU access enabled
    BufferCache(bool allow_cpu_access = true) : _allow_cpu_access(allow_cpu_access) {}

    /// Get Buffer associated to this id
    /// @param id: unique buffer id (typically a pointer or handle)
    /// @return pointer to Buffer object associated to this id if present else nullptr
    Buffer* get(Id buffer_id)
    {
        auto item = _buffers.find(buffer_id);
        return item == _buffers.end() ? nullptr : &item->second;
    }
    
    /// Add Buffer for the specified address and size.
    /// @param id: unique buffer id
    /// @param data_address: pointer to buffer data. Must be aligned to Allocator::align
    /// @param data_size: size of buffer data. Must be a multiple of Allocator::align
    /// @return pointer to a Buffer object referencing the specified address
    Buffer* add(Id buffer_id, const void* data_address, size_t data_size)
    {
        Buffer buffer(const_cast<void*>(data_address), data_size, _allow_cpu_access);
        return &_buffers.emplace(buffer_id, std::move(buffer)).first->second;
    }
    
    /// Get Buffer associated to this id if it exists, else create a new Buffer
    Buffer* get(Id buffer_id, const void* data_address, size_t data_size) {
        Buffer* buffer = get(buffer_id);
        return buffer? buffer : add(buffer_id, data_address, data_size);
    }

    /// @return number of buffers in the cache
    size_t size() const { return _buffers.size(); }

    /// Clear the cache
    void clear() { _buffers.clear(); }
    
    /// Iterate buffers
    /// @return buffers map iterator
    typename Map::iterator begin() { return _buffers.begin(); }
    typename Map::iterator end() { return _buffers.end(); }
    
protected:
    bool _allow_cpu_access{};
    Map _buffers;
};


}  // namespace synap
}  // namespace synaptics
