/* ************************************************************************
 * Copyright (C) 2016-2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
 * ies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
 * PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
 * CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ************************************************************************ */
#pragma once

#include "int64_helpers.hpp"
#include "logging.hpp"
#include "rocblas_hbmv.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_hbmv_name[] = "unknown";
    template <>
    constexpr char rocblas_hbmv_name<rocblas_float_complex>[] = ROCBLAS_API_STR(rocblas_chbmv);
    template <>
    constexpr char rocblas_hbmv_name<rocblas_double_complex>[] = ROCBLAS_API_STR(rocblas_zhbmv);

    template <typename API_INT, typename T>
    rocblas_status rocblas_hbmv_impl(rocblas_handle handle,
                                     rocblas_fill   uplo,
                                     API_INT        n,
                                     API_INT        k,
                                     const T*       alpha,
                                     const T*       A,
                                     API_INT        lda,
                                     const T*       x,
                                     API_INT        incx,
                                     const T*       beta,
                                     T*             y,
                                     API_INT        incy)
    {
        if(!handle)
            return rocblas_status_invalid_handle;
        RETURN_ZERO_DEVICE_MEMORY_SIZE_IF_QUERIED(handle);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;
        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto uplo_letter = rocblas_fill_letter(uplo);

            if(layer_mode & rocblas_layer_mode_log_trace)
                log_trace(handle,
                          rocblas_hbmv_name<T>,
                          uplo,
                          n,
                          k,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          A,
                          lda,
                          x,
                          incx,
                          LOG_TRACE_SCALAR_VALUE(handle, beta),
                          y,
                          incy);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          ROCBLAS_API_BENCH " -f hbmv -r",
                          rocblas_precision_string<T>,
                          "--uplo",
                          uplo_letter,
                          "-n",
                          n,
                          "-k",
                          k,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--lda",
                          lda,
                          "--incx",
                          incx,
                          LOG_BENCH_SCALAR_VALUE(handle, beta),
                          "--incy",
                          incy);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_hbmv_name<T>,
                            "uplo",
                            uplo_letter,
                            "N",
                            n,
                            "K",
                            k,
                            "lda",
                            lda,
                            "incx",
                            incx,
                            "incy",
                            incy);
        }

        rocblas_status arg_status = rocblas_hbmv_arg_check<API_INT>(
            handle, uplo, n, k, alpha, A, 0, lda, 0, x, 0, incx, 0, beta, y, 0, incy, 0, 1);
        if(arg_status != rocblas_status_continue)
            return arg_status;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status hbmv_check_numerics_status
                = rocblas_hbmv_check_numerics(rocblas_hbmv_name<T>,
                                              handle,
                                              n,
                                              k,
                                              A,
                                              0,
                                              lda,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              y,
                                              0,
                                              incy,
                                              0,
                                              1,
                                              check_numerics,
                                              is_input);
            if(hbmv_check_numerics_status != rocblas_status_success)
                return hbmv_check_numerics_status;
        }

        rocblas_status status = ROCBLAS_API(rocblas_internal_hbmv_launcher)(
            handle, uplo, n, k, alpha, A, 0, lda, 0, x, 0, incx, 0, beta, y, 0, incy, 0, 1);
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status hbmv_check_numerics_status
                = rocblas_hbmv_check_numerics(rocblas_hbmv_name<T>,
                                              handle,
                                              n,
                                              k,
                                              A,
                                              0,
                                              lda,
                                              0,
                                              x,
                                              0,
                                              incx,
                                              0,
                                              y,
                                              0,
                                              incy,
                                              0,
                                              1,
                                              check_numerics,
                                              is_input);
            if(hbmv_check_numerics_status != rocblas_status_success)
                return hbmv_check_numerics_status;
        }
        return status;
    }
} // namespace

/*
* ===========================================================================
*    C wrapper
* ===========================================================================
*/

#ifdef IMPL
#error IMPL ALREADY DEFINED
#endif

#define IMPL(routine_name_, TI_, T_)                                    \
    rocblas_status routine_name_(rocblas_handle handle,                 \
                                 rocblas_fill   uplo,                   \
                                 TI_            n,                      \
                                 TI_            k,                      \
                                 const T_*      alpha,                  \
                                 const T_*      A,                      \
                                 TI_            lda,                    \
                                 const T_*      x,                      \
                                 TI_            incx,                   \
                                 const T_*      beta,                   \
                                 T_*            y,                      \
                                 TI_            incy)                   \
    try                                                                 \
    {                                                                   \
        return rocblas_hbmv_impl<TI_, T_>(                              \
            handle, uplo, n, k, alpha, A, lda, x, incx, beta, y, incy); \
    }                                                                   \
    catch(...)                                                          \
    {                                                                   \
        return exception_to_rocblas_status();                           \
    }

#define INST_HBMV_C_API(TI_)                                       \
    extern "C" {                                                   \
    IMPL(ROCBLAS_API(rocblas_chbmv), TI_, rocblas_float_complex);  \
    IMPL(ROCBLAS_API(rocblas_zhbmv), TI_, rocblas_double_complex); \
    } // extern "C"