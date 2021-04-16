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
/// Synap allocator.
///

#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

namespace synaptics {
namespace synap {


/// Buffer allocator.
/// Allows to allocate aligned memory from different areas.
/// Memory must be allocated such that it completely includes all the cache-lines used for the
/// actual data. This ensures that no cache-line used for data is also used for something else.
class Allocator {
public:
    struct Memory {
        /// Aligned memory pointer
        void* address;
        
        /// Memory block handle, allocator specific.
        uintptr_t handle;
    };
    
    /// Allocate memory.
    /// @param size: required memory size in bytes
    /// @return allocated memory information
    virtual Memory alloc(size_t size) = 0;

    /// Deallocate memory.
    /// @param handle: memory handle to deallocate
    virtual void dealloc(const uintptr_t handle) = 0;

    virtual ~Allocator() {}


    /// Required alignment. This corresponds to the size of the cache line.
    static constexpr size_t alignment = 64;

    /// @return val rounded upward to alignment
    static uintptr_t align(uintptr_t val) { return (val + alignment - 1) & ~(alignment - 1); }

    /// @return addr rounded upward to alignment
    static void* align(void* addr) { return (void*)align((uintptr_t)addr); }
    static const void* align(const void* addr) { return (void*)align((uintptr_t)addr); }
};


/// Get a pointer to the global standard (malloc-based) allocator.
/// @return pointer to standard allocator
Allocator* std_allocator();

/// Get a pointer to the global CMA allocator.
/// @return pointer to the CMA allocator if available, else nullptr
Allocator* cma_allocator();

}  // namespace synap
}  // namespace synaptics
