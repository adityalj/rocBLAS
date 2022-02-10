/* ************************************************************************
 * Copyright 2016-2022 Advanced Micro Devices, Inc.
 * ************************************************************************ */
#include "logging.hpp"
#include "rocblas_gemv.hpp"

namespace
{
    template <typename>
    constexpr char rocblas_gemv_name[] = "unknown";
    template <>
    constexpr char rocblas_gemv_name<float>[] = "rocblas_sgemv_batched";
    template <>
    constexpr char rocblas_gemv_name<double>[] = "rocblas_dgemv_batched";
    template <>
    constexpr char rocblas_gemv_name<rocblas_float_complex>[] = "rocblas_cgemv_batched";
    template <>
    constexpr char rocblas_gemv_name<rocblas_double_complex>[] = "rocblas_zgemv_batched";

    template <typename T>
    rocblas_status rocblas_gemv_batched_impl(rocblas_handle    handle,
                                             rocblas_operation transA,
                                             rocblas_int       m,
                                             rocblas_int       n,
                                             const T*          alpha,
                                             const T* const    A[],
                                             rocblas_int       lda,
                                             const T* const    x[],
                                             rocblas_int       incx,
                                             const T*          beta,
                                             T* const          y[],
                                             rocblas_int       incy,
                                             rocblas_int       batch_count)
    {
        if(!handle)
            return rocblas_status_invalid_handle;

        size_t dev_bytes
            = rocblas_internal_gemv_kernel_workspace_size<T>(transA, m, n, batch_count);
        if(handle->is_device_memory_size_query())
            return handle->set_optimal_device_memory_size(dev_bytes);

        auto layer_mode     = handle->layer_mode;
        auto check_numerics = handle->check_numerics;

        if(layer_mode
           & (rocblas_layer_mode_log_trace | rocblas_layer_mode_log_bench
              | rocblas_layer_mode_log_profile))
        {
            auto transA_letter = rocblas_transpose_letter(transA);

            if(layer_mode & rocblas_layer_mode_log_trace)
                log_trace(handle,
                          rocblas_gemv_name<T>,
                          transA,
                          m,
                          n,
                          LOG_TRACE_SCALAR_VALUE(handle, alpha),
                          A,
                          lda,
                          x,
                          incx,
                          LOG_TRACE_SCALAR_VALUE(handle, beta),
                          y,
                          incy,
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_bench)
                log_bench(handle,
                          "./rocblas-bench -f gemv_batched -r",
                          rocblas_precision_string<T>,
                          "--transposeA",
                          transA_letter,
                          "-m",
                          m,
                          "-n",
                          n,
                          LOG_BENCH_SCALAR_VALUE(handle, alpha),
                          "--lda",
                          lda,
                          "--incx",
                          incx,
                          LOG_BENCH_SCALAR_VALUE(handle, beta),
                          "--incy",
                          incy,
                          "--batch_count",
                          batch_count);

            if(layer_mode & rocblas_layer_mode_log_profile)
                log_profile(handle,
                            rocblas_gemv_name<T>,
                            "transA",
                            transA_letter,
                            "M",
                            m,
                            "N",
                            n,
                            "lda",
                            lda,
                            "incx",
                            incx,
                            "incy",
                            incy,
                            "batch_count",
                            batch_count);
        }

        if(m < 0 || n < 0 || lda < m || lda < 1 || !incx || !incy || batch_count < 0)
            return rocblas_status_invalid_size;

        if(!m || !n || !batch_count)
            return rocblas_status_success;

        if(!alpha || !beta)
            return rocblas_status_invalid_pointer;

        if(handle->pointer_mode == rocblas_pointer_mode_host && !*alpha)
        {
            if(*beta == 1)
                return rocblas_status_success;
        }
        else
        {
            if(!A || !x)
                return rocblas_status_invalid_pointer;
        }

        if(!y)
            return rocblas_status_invalid_pointer;

        rocblas_status perf_status = rocblas_status_success;
        auto           w_mem       = handle->device_malloc(dev_bytes);
        if(!w_mem)
            perf_status = rocblas_status_perf_degraded;

        if(check_numerics)
        {
            bool           is_input = true;
            rocblas_status gemv_check_numerics_status
                = rocblas_gemv_check_numerics(rocblas_gemv_name<T>,
                                              handle,
                                              transA,
                                              m,
                                              n,
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
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(gemv_check_numerics_status != rocblas_status_success)
                return gemv_check_numerics_status;
        }

        rocblas_status status = rocblas_internal_gemv_template<T>(handle,
                                                                  transA,
                                                                  m,
                                                                  n,
                                                                  alpha,
                                                                  0,
                                                                  A,
                                                                  0,
                                                                  lda,
                                                                  0,
                                                                  x,
                                                                  0,
                                                                  incx,
                                                                  0,
                                                                  beta,
                                                                  0,
                                                                  y,
                                                                  0,
                                                                  incy,
                                                                  0,
                                                                  batch_count,
                                                                  (T*)w_mem);

        status = (status != rocblas_status_success) ? status : perf_status;
        if(status != rocblas_status_success)
            return status;

        if(check_numerics)
        {
            bool           is_input = false;
            rocblas_status gemv_check_numerics_status
                = rocblas_gemv_check_numerics(rocblas_gemv_name<T>,
                                              handle,
                                              transA,
                                              m,
                                              n,
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
                                              batch_count,
                                              check_numerics,
                                              is_input);
            if(gemv_check_numerics_status != rocblas_status_success)
                return gemv_check_numerics_status;
        }
        return status;
    }
} // namespace

/*
 * ===========================================================================
 *    C wrapper
 * ===========================================================================
 */

extern "C" {

rocblas_status rocblas_sgemv_batched(rocblas_handle     handle,
                                     rocblas_operation  transA,
                                     rocblas_int        m,
                                     rocblas_int        n,
                                     const float*       alpha,
                                     const float* const A[],
                                     rocblas_int        lda,
                                     const float* const x[],
                                     rocblas_int        incx,
                                     const float*       beta,
                                     float* const       y[],
                                     rocblas_int        incy,
                                     rocblas_int        batch_count)
try
{
    return rocblas_gemv_batched_impl(
        handle, transA, m, n, alpha, A, lda, x, incx, beta, y, incy, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_dgemv_batched(rocblas_handle      handle,
                                     rocblas_operation   transA,
                                     rocblas_int         m,
                                     rocblas_int         n,
                                     const double*       alpha,
                                     const double* const A[],
                                     rocblas_int         lda,
                                     const double* const x[],
                                     rocblas_int         incx,
                                     const double*       beta,
                                     double* const       y[],
                                     rocblas_int         incy,
                                     rocblas_int         batch_count)
try
{
    return rocblas_gemv_batched_impl(
        handle, transA, m, n, alpha, A, lda, x, incx, beta, y, incy, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_cgemv_batched(rocblas_handle                     handle,
                                     rocblas_operation                  transA,
                                     rocblas_int                        m,
                                     rocblas_int                        n,
                                     const rocblas_float_complex*       alpha,
                                     const rocblas_float_complex* const A[],
                                     rocblas_int                        lda,
                                     const rocblas_float_complex* const x[],
                                     rocblas_int                        incx,
                                     const rocblas_float_complex*       beta,
                                     rocblas_float_complex* const       y[],
                                     rocblas_int                        incy,
                                     rocblas_int                        batch_count)
try
{
    return rocblas_gemv_batched_impl(
        handle, transA, m, n, alpha, A, lda, x, incx, beta, y, incy, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

rocblas_status rocblas_zgemv_batched(rocblas_handle                      handle,
                                     rocblas_operation                   transA,
                                     rocblas_int                         m,
                                     rocblas_int                         n,
                                     const rocblas_double_complex*       alpha,
                                     const rocblas_double_complex* const A[],
                                     rocblas_int                         lda,
                                     const rocblas_double_complex* const x[],
                                     rocblas_int                         incx,
                                     const rocblas_double_complex*       beta,
                                     rocblas_double_complex* const       y[],
                                     rocblas_int                         incy,
                                     rocblas_int                         batch_count)
try
{
    return rocblas_gemv_batched_impl(
        handle, transA, m, n, alpha, A, lda, x, incx, beta, y, incy, batch_count);
}
catch(...)
{
    return exception_to_rocblas_status();
}

} // extern "C"
