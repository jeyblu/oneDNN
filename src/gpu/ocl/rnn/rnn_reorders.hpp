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

#ifndef GPU_OCL_RNN_RNN_REORDERS_HPP
#define GPU_OCL_RNN_RNN_REORDERS_HPP

#include "common/c_types_map.hpp"
#include "common/memory.hpp"
#include "common/primitive.hpp"
#include "common/utils.hpp"
#include "gpu/compute/compute.hpp"
#include "gpu/gpu_primitive.hpp"
#include "gpu/gpu_reorder_pd.hpp"
#include "gpu/gpu_resource.hpp"
#include "gpu/ocl/ocl_utils.hpp"
#include "gpu/primitive_conf.hpp"

namespace dnnl {
namespace impl {
namespace gpu {
namespace ocl {

struct rnn_weights_reorder_t : public gpu_primitive_t {
    using gpu_primitive_t::gpu_primitive_t;
    struct pd_t : public reorder_pd_t {
        using reorder_pd_t::reorder_pd_t;

        DECLARE_COMMON_PD_T("cross_engine::rnn", rnn_weights_reorder_t);

        status_t init(
                engine_t *engine, engine_t *src_engine, engine_t *dst_engine) {
            // Note: currently rnn_u8s8_compensation and rnn_s8s8_compensation
            // have common bit so we have to perform additional checks to
            // separate these two cases
            if (IMPLICATION(dst_md()->extra.flags
                                & memory_extra_flags::rnn_u8s8_compensation,
                        types::extra_flag_rnn_s8s8_compensation_is_set(
                                dst_md()->extra.flags)))
                return status::unimplemented;

            bool args_ok = true
                    && utils::one_of(src_engine->kind(), engine_kind::gpu,
                            engine_kind::cpu)
                    && dst_engine->kind() == engine_kind::gpu;
            if (!args_ok) return status::unimplemented;

            auto *compute_engine
                    = utils::downcast<compute::compute_engine_t *>(dst_engine);

            args_ok = args_ok
                    && compute_engine->mayiuse(
                            compute::device_ext_t::intel_subgroups)
                    && IMPLICATION(
                            utils::one_of(data_type::f16, src_md()->data_type,
                                    dst_md()->data_type),
                            true
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::khr_fp16)
                                    && compute_engine->mayiuse(
                                            compute::device_ext_t::
                                                    intel_subgroups_short));

            auto status = init_conf(engine);
            if (status != status::success) return status;
            init_scratchpad();
            return status;
        }

        status_t init_conf(engine_t *engine);
        status_t init_kernel_ctx(compute::kernel_ctx_t &kernel_ctx) const;

        rnn_reorder_conf_t conf;

    private:
        DECLARE_GPU_REORDER_CREATE();

        void init_scratchpad() {
            auto scratchpad = scratchpad_registry().registrar();

            if (conf.do_reorder) {
                size_t sz = conf.nelems;
                scratchpad.book(memory_tracking::names::key_reorder_rnn_space,
                        sz, sizeof(float), OCL_BUFFER_ALIGNMENT);
            }
        }
    };

    status_t init(engine_t *engine) override {
        compute::kernel_ctx_t kernel_ctx;

        auto status = pd()->init_kernel_ctx(kernel_ctx);
        if (status != status::success) return status;

        create_kernel(engine, &kernel_, "wei_reorder", kernel_ctx);
        if (!kernel_) return status::runtime_error;
        return status::success;
    }

    status_t execute(const exec_ctx_t &ctx) const override;

private:
    const pd_t *pd() const { return (const pd_t *)primitive_t::pd().get(); }
    compute::kernel_t kernel_;
};

} // namespace ocl
} // namespace gpu
} // namespace impl
} // namespace dnnl

#endif
