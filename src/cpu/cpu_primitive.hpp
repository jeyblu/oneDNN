/*******************************************************************************
* Copyright 2019-2022 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef CPU_CPU_PRIMITIVE_HPP
#define CPU_CPU_PRIMITIVE_HPP

#include <assert.h>

#include "oneapi/dnnl/dnnl_types.h"

#include "common/c_types_map.hpp"
#include "common/primitive_attr.hpp"
#include "common/primitive_exec_types.hpp"
#include "common/utils.hpp"
#include "common/z_magic.hpp"

#define DEFINE_SCALES_BUFFER_ATTR_ARG(attr, scales, arg) \
    alignas(16) float CONCAT2(scales, _buf16)[16] = {0}; \
    const float *scales {nullptr}; \
    if ((attr)) { \
        if ((attr)->output_scales_.has_default_values()) { \
            utils::array_set(CONCAT2(scales, _buf16), 1.0f, 16); \
            scales = CONCAT2(scales, _buf16); \
        } else { \
            scales = CTX_IN_MEM(const float *, arg); \
            if (scales == nullptr) return status::invalid_arguments; \
            const auto scales_d = ctx.memory_mdw(arg); \
            bool ok = scales_d.data_type() == data_type::f32 \
                    && scales_d.ndims() == 1; \
            if (!ok) return status::invalid_arguments; \
            if (scales_d.dims()[0] == 1) { \
                utils::array_set(CONCAT2(scales, _buf16), scales[0], 16); \
                scales = CONCAT2(scales, _buf16); \
            } \
        } \
    } \
    MAYBE_UNUSED(scales);

#define DEFINE_SCALES_BUFFER_ATTR(attr, scales) \
    DEFINE_SCALES_BUFFER_ATTR_ARG(attr, scales, DNNL_ARG_ATTR_OUTPUT_SCALES);

#define DEFINE_SCALES_BUFFER(scales) \
    DEFINE_SCALES_BUFFER_ATTR(pd()->attr(), scales)

#define DEFINE_ZERO_POINTS_BUFFER(zero_points_ptr, mem_arg) \
    int32_t CONCAT2(default_zero_point_, mem_arg) = 0; \
    const int32_t *zero_points_ptr \
            = pd()->attr()->zero_points_.defined(mem_arg) \
            ? &CONCAT2(default_zero_point_, mem_arg) \
            : CTX_IN_MEM( \
                    const int32_t *, DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
    if (zero_points_ptr == nullptr) return status::invalid_arguments; \
    MAYBE_UNUSED(zero_points_ptr);

#define ASSIGN_ARG_SCALE_VALUE(scale, mem_arg) \
    alignas(16) float CONCAT2(CONCAT2(scales, _buf16), mem_arg)[16] = {0}; \
    if (pd()->attr()->scales_.get(mem_arg).has_default_values()) { \
        utils::array_set(CONCAT2(CONCAT2(scales, _buf16), mem_arg), 1.0f, 16); \
        scale = CONCAT2(CONCAT2(scales, _buf16), mem_arg); \
    } else { \
        const auto scale_d = ctx.memory_mdw(DNNL_ARG_ATTR_SCALES | mem_arg); \
        bool ok = scale_d.data_type() == data_type::f32 \
                && scale_d.ndims() == 1 && scale_d.dims()[0] == 1; \
        if (!ok) return status::invalid_arguments; \
        const float *scale_p \
                = CTX_IN_MEM(const float *, DNNL_ARG_ATTR_SCALES | mem_arg); \
        if (scale_p == nullptr) return status::invalid_arguments; \
        scale = scale_p; \
    }

#define DEFINE_ZERO_POINT_VALUE(zero_point, mem_arg) \
    int32_t zero_point = 0; \
    if (!pd()->attr()->zero_points_.has_default_values(mem_arg)) { \
        const auto zero_points_d \
                = ctx.memory_mdw(DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
        bool ok = zero_points_d.data_type() == data_type::s32 \
                && zero_points_d.ndims() == 1 && zero_points_d.dims()[0] == 1; \
        if (!ok) return status::invalid_arguments; \
        const int32_t *zero_points_ptr = CTX_IN_MEM( \
                const int32_t *, DNNL_ARG_ATTR_ZERO_POINTS | mem_arg); \
        if (zero_points_ptr == nullptr) return status::invalid_arguments; \
        zero_point = *zero_points_ptr; \
    } \
    MAYBE_UNUSED(zero_point);

#endif // CPU_CPU_PRIMITIVE_HPP
