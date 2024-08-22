/* ************************************************************************
 * Copyright (C) 2018-2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include "testing_common.hpp"

#include "blas3/rocblas_trsm.hpp"
#include "src64/blas3/rocblas_trsm_64.hpp"

#define ERROR_EPS_MULTIPLIER 40
#define RESIDUAL_EPS_MULTIPLIER 40

template <typename T>
void testing_trsm_bad_arg(const Arguments& arg)
{
    auto rocblas_trsm_fn = arg.api & c_API_FORTRAN ? rocblas_trsm<T, true> : rocblas_trsm<T, false>;
    auto rocblas_trsm_fn_64
        = arg.api & c_API_FORTRAN ? rocblas_trsm_64<T, true> : rocblas_trsm_64<T, false>;

    for(auto pointer_mode : {rocblas_pointer_mode_host, rocblas_pointer_mode_device})
    {
        rocblas_local_handle handle{arg};
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, pointer_mode));

        const int64_t M   = 100;
        const int64_t N   = 100;
        const int64_t lda = 100;
        const int64_t ldb = 100;

        DEVICE_MEMCHECK(device_vector<T>, alpha_d, (1));
        DEVICE_MEMCHECK(device_vector<T>, zero_d, (1));

        const T alpha_h(1), zero_h(0);

        const T* alpha = &alpha_h;
        const T* zero  = &zero_h;

        if(pointer_mode == rocblas_pointer_mode_device)
        {
            CHECK_HIP_ERROR(hipMemcpy(alpha_d, alpha, sizeof(*alpha), hipMemcpyHostToDevice));
            alpha = alpha_d;
            CHECK_HIP_ERROR(hipMemcpy(zero_d, zero, sizeof(*zero), hipMemcpyHostToDevice));
            zero = zero_d;
        }

        const rocblas_side      side   = rocblas_side_left;
        const rocblas_fill      uplo   = rocblas_fill_upper;
        const rocblas_operation transA = rocblas_operation_none;
        const rocblas_diagonal  diag   = rocblas_diagonal_non_unit;

        int64_t K = side == rocblas_side_left ? M : N;

        // Allocate device memory
        DEVICE_MEMCHECK(device_matrix<T>, dA, (K, K, lda));
        DEVICE_MEMCHECK(device_matrix<T>, dB, (M, N, ldb));

        // check for invalid enum
        DAPI_EXPECT(rocblas_status_invalid_value,
                    rocblas_trsm_fn,
                    (handle, rocblas_side_both, uplo, transA, diag, M, N, alpha, dA, lda, dB, ldb));

        DAPI_EXPECT(rocblas_status_invalid_value,
                    rocblas_trsm_fn,
                    (handle,
                     side,
                     (rocblas_fill)rocblas_side_both,
                     transA,
                     diag,
                     M,
                     N,
                     alpha,
                     dA,
                     lda,
                     dB,
                     ldb));

        DAPI_EXPECT(rocblas_status_invalid_value,
                    rocblas_trsm_fn,
                    (handle,
                     side,
                     uplo,
                     (rocblas_operation)rocblas_side_both,
                     diag,
                     M,
                     N,
                     alpha,
                     dA,
                     lda,
                     dB,
                     ldb));

        DAPI_EXPECT(rocblas_status_invalid_value,
                    rocblas_trsm_fn,
                    (handle,
                     side,
                     uplo,
                     transA,
                     (rocblas_diagonal)rocblas_side_both,
                     M,
                     N,
                     alpha,
                     dA,
                     lda,
                     dB,
                     ldb));

        // check for invalid size
        DAPI_EXPECT(rocblas_status_invalid_size,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, -1, N, alpha, dA, lda, dB, ldb));

        DAPI_EXPECT(rocblas_status_invalid_size,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, -1, alpha, dA, lda, dB, ldb));

        /// check for invalid leading dimension
        DAPI_EXPECT(rocblas_status_invalid_size,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, N, alpha, dA, lda, dB, M - 1));

        DAPI_EXPECT(
            rocblas_status_invalid_size,
            rocblas_trsm_fn,
            (handle, rocblas_side_left, uplo, transA, diag, M, N, alpha, dA, M - 1, dB, ldb));

        DAPI_EXPECT(
            rocblas_status_invalid_size,
            rocblas_trsm_fn,
            (handle, rocblas_side_right, uplo, transA, diag, M, N, alpha, dA, N - 1, dB, ldb));

        // check that nullpointer gives rocblas_status_invalid_handle or rocblas_status_invalid_pointer
        DAPI_EXPECT(rocblas_status_invalid_handle,
                    rocblas_trsm_fn,
                    (nullptr, side, uplo, transA, diag, M, N, alpha, dA, lda, dB, ldb));

        DAPI_EXPECT(rocblas_status_invalid_pointer,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, N, nullptr, dA, lda, dB, ldb));

        if(pointer_mode == rocblas_pointer_mode_host)
        {
            DAPI_EXPECT(rocblas_status_invalid_pointer,
                        rocblas_trsm_fn,
                        (handle, side, uplo, transA, diag, M, N, alpha, nullptr, lda, dB, ldb));
        }

        DAPI_EXPECT(rocblas_status_invalid_pointer,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, N, alpha, dA, lda, nullptr, ldb));

        // quick return: If alpha==0, then A can be nullptr without error
        DAPI_EXPECT(rocblas_status_success,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, N, zero, nullptr, lda, dB, ldb));

        // quick return: If M==0, then all pointers can be nullptr without error
        DAPI_EXPECT(rocblas_status_success,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, 0, N, nullptr, nullptr, lda, nullptr, ldb));

        // quick return: If N==0, then all pointers can be nullptr without error
        DAPI_EXPECT(rocblas_status_success,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, 0, nullptr, nullptr, lda, nullptr, ldb));
    }
}

template <typename T>
void testing_trsm_internal_interfaces(const Arguments& arg)
{
    // testing rocblas_internal_trsm_workspace_max_size to ensure that the sizes it gives
    // is large enough for all/various sizes below the sizes given

    int64_t M           = arg.M;
    int64_t N           = arg.N;
    int64_t batch_count = arg.batch_count;

    rocblas_side side = char2rocblas_side(arg.side);

    size_t w_x_tmp_size, w_invA_size, w_x_tmp_size_backup;

    CHECK_ROCBLAS_ERROR(rocblas_internal_trsm_workspace_max_size_64<T>(
        side, M, N, batch_count, &w_x_tmp_size, &w_invA_size, &w_x_tmp_size_backup));

    // test out below for various sizes below M and N
    for(int64_t m_smaller = M; m_smaller > 0; m_smaller--)
    {
        for(int64_t n_smaller = N; n_smaller > 0; n_smaller--)
        {
            size_t w_x_tmp_size2, w_x_tmp_arr_size2, w_invA_size2, w_invA_arr_size2,
                w_x_tmp_size_backup2;

            // This is implementation-dependent, but currently we /may/ use less memory with "skinny"
            // matrices when transA == non-transpose.
            // Setting this to transpose will always allocate >= non-transpose invokations, so good
            // for this test
            rocblas_operation transA     = rocblas_operation_transpose;
            rocblas_status    mem_status = rocblas_internal_trsm_workspace_size<T>(
                side,
                transA,
                m_smaller,
                n_smaller,
                batch_count, // not bothering to test smaller batch_counts
                0, // not supporting supplied invA for max_size fn
                &w_x_tmp_size2,
                &w_x_tmp_arr_size2,
                &w_invA_size2,
                &w_invA_arr_size2,
                &w_x_tmp_size_backup2);

            if(mem_status != rocblas_status_success && mem_status != rocblas_status_continue)
                CHECK_ROCBLAS_ERROR(mem_status);

#ifdef GOOGLE_TEST
            ASSERT_TRUE(w_x_tmp_size2 <= w_x_tmp_size && w_invA_size2 <= w_invA_size
                        && w_x_tmp_size_backup2 <= w_x_tmp_size_backup);
#endif
        }
    }
}

template <typename T>
void testing_trsm(const Arguments& arg)
{
    auto rocblas_trsm_fn = arg.api & c_API_FORTRAN ? rocblas_trsm<T, true> : rocblas_trsm<T, false>;
    auto rocblas_trsm_fn_64
        = arg.api & c_API_FORTRAN ? rocblas_trsm_64<T, true> : rocblas_trsm_64<T, false>;

    int64_t M   = arg.M;
    int64_t N   = arg.N;
    int64_t lda = arg.lda;
    int64_t ldb = arg.ldb;

    char char_side   = arg.side;
    char char_uplo   = arg.uplo;
    char char_transA = arg.transA;
    char char_diag   = arg.diag;
    T    alpha_h     = arg.get_alpha<T>();

    bool HMM = arg.HMM;

    rocblas_side      side   = char2rocblas_side(char_side);
    rocblas_fill      uplo   = char2rocblas_fill(char_uplo);
    rocblas_operation transA = char2rocblas_operation(char_transA);
    rocblas_diagonal  diag   = char2rocblas_diagonal(char_diag);

    int64_t K = side == rocblas_side_left ? M : N;

    rocblas_local_handle handle{arg};

    // check here to prevent undefined memory allocation error
    bool invalid_size = M < 0 || N < 0 || lda < K || ldb < M;
    if(invalid_size)
    {
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        DAPI_EXPECT(rocblas_status_invalid_size,
                    rocblas_trsm_fn,
                    (handle, side, uplo, transA, diag, M, N, nullptr, nullptr, lda, nullptr, ldb));

        return;
    }

    // Naming: `h` is in CPU (host) memory(eg hA), `d` is in GPU (device) memory (eg dA).
    // Allocate host memory
    HOST_MEMCHECK(host_matrix<T>, hA, (K, K, lda));
    HOST_MEMCHECK(host_matrix<T>, hB, (M, N, M)); // save memory when large ldb
    HOST_MEMCHECK(host_matrix<T>, hX, (M, N, ldb));
    HOST_MEMCHECK(host_matrix<T>, hXorB_1, (M, N, ldb));

    // Allocate device memory
    DEVICE_MEMCHECK(device_matrix<T>, dA, (K, K, lda, HMM));
    DEVICE_MEMCHECK(device_matrix<T>, dXorB, (M, N, ldb, HMM));
    DEVICE_MEMCHECK(device_vector<T>, alpha_d, (1, 1, HMM));

    // Initialize data on host memory
    rocblas_init_matrix(hA,
                        arg,
                        rocblas_client_never_set_nan,
                        rocblas_client_diagonally_dominant_triangular_matrix,
                        true);
    rocblas_init_matrix(
        hX, arg, rocblas_client_never_set_nan, rocblas_client_general_matrix, false, true);

    //  make hA unit diagonal if diag == rocblas_diagonal_unit
    if(diag == rocblas_diagonal_unit)
    {
        make_unit_diagonal(uplo, (T*)hA, lda, K);
    }

    copy_matrix_with_different_leading_dimensions(hX, hB);

    // Calculate hB = hA*hX;
    ref_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hB, M);
    copy_matrix_with_different_leading_dimensions(hB, hXorB_1);

    // copy data from CPU to device
    CHECK_HIP_ERROR(dA.transfer_from(hA));
    CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

    double error_eps_multiplier    = ERROR_EPS_MULTIPLIER;
    double residual_eps_multiplier = RESIDUAL_EPS_MULTIPLIER;
    double eps                     = std::numeric_limits<real_t<T>>::epsilon();
    double err_host                = 0.0;
    double err_device              = 0.0;

    if(!ROCBLAS_REALLOC_ON_DEMAND)
    {
        // Compute size
        CHECK_ROCBLAS_ERROR(rocblas_start_device_memory_size_query(handle));
        DAPI_CHECK_ALLOC_QUERY(
            rocblas_trsm_fn,
            (handle, side, uplo, transA, diag, M, N, &alpha_h, dA, lda, dXorB, ldb));

        size_t size;
        CHECK_ROCBLAS_ERROR(rocblas_stop_device_memory_size_query(handle, &size));

        // Allocate memory
        CHECK_ROCBLAS_ERROR(rocblas_set_device_memory_size(handle, size));
    }

    if(arg.unit_check || arg.norm_check)
    {
        if(arg.pointer_mode_host)
        {
            // calculate dXorB <- A^(-1) B   rocblas_device_pointer_host
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));
            CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));

            handle.pre_test(arg);
            if(arg.api != INTERNAL)
            {
                DAPI_CHECK(rocblas_trsm_fn,
                           (handle, side, uplo, transA, diag, M, N, &alpha_h, dA, lda, dXorB, ldb));
            }
            else
            {
                // NOTE: not testing internal 64-bit API as of now
                // internal function requires us to supply temporary memory ourselves
                constexpr bool    BATCHED        = false;
                constexpr int64_t batch_count    = 1;
                bool              optimal_mem    = true;
                int64_t           supp_invA_size = 0; // used for trsm_ex

                // first exported internal interface - calculate how much mem is needed
                size_t w_x_tmp_size, w_x_tmp_arr_size, w_invA_size, w_invA_arr_size,
                    w_x_tmp_size_backup;
                rocblas_status mem_status
                    = rocblas_internal_trsm_workspace_size<T>(side,
                                                              transA,
                                                              M,
                                                              N,
                                                              batch_count,
                                                              supp_invA_size,
                                                              &w_x_tmp_size,
                                                              &w_x_tmp_arr_size,
                                                              &w_invA_size,
                                                              &w_invA_arr_size,
                                                              &w_x_tmp_size_backup);

                if(mem_status != rocblas_status_success && mem_status != rocblas_status_continue)
                    CHECK_ROCBLAS_ERROR(mem_status);

                // allocate memory ourselves
                DEVICE_MEMCHECK(device_vector<T>, w_mem_x_tmp, (w_x_tmp_size / sizeof(T)));
                DEVICE_MEMCHECK(device_vector<T>, w_mem_x_tmp_arr, (w_x_tmp_arr_size / sizeof(T*)));
                DEVICE_MEMCHECK(device_vector<T>, w_mem_invA, (w_invA_size / sizeof(T)));
                DEVICE_MEMCHECK(device_vector<T>, w_mem_invA_arr, (w_invA_arr_size / sizeof(T*)));

                // using ldc/ldd as offsets
                rocblas_stride strideA = 0, strideB = 0;
                rocblas_stride offsetA = arg.ldc, offsetB = arg.ldd;

                CHECK_ROCBLAS_ERROR(rocblas_internal_trsm_template(handle,
                                                                   side,
                                                                   uplo,
                                                                   transA,
                                                                   diag,
                                                                   M,
                                                                   N,
                                                                   &alpha_h,
                                                                   (const T*)dA + offsetA,
                                                                   -offsetA,
                                                                   lda,
                                                                   strideA,
                                                                   (T*)dXorB + offsetB,
                                                                   -offsetB,
                                                                   ldb,
                                                                   strideB,
                                                                   batch_count,
                                                                   optimal_mem,
                                                                   (void*)w_mem_x_tmp,
                                                                   (void*)w_mem_x_tmp_arr,
                                                                   (void*)w_mem_invA,
                                                                   (void*)w_mem_invA_arr));
            }
            handle.post_test(arg);

            CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));

            // doing unit tests here to save memory by having hB just use M as leading dimension,
            // need to reuse hXorB for hipMemcpy later
            if(alpha_h == 0)
            {
                // expecting 0 output, set hX == 0
                rocblas_init_zero((T*)hX, M, N, ldb);

                if(arg.unit_check)
                    unit_check_general<T>(M, N, ldb, hX, hXorB_1);
                if(arg.norm_check)
                    err_host = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_1));
            }
            else
            {
                //computed result is in hx_or_b, so forward error is E = hx - hx_or_b
                // calculate vector-induced-norm 1 of matrix E
                err_host = matrix_norm_1<T>(M, N, ldb, hX, hXorB_1);

                if(arg.unit_check)
                    trsm_err_res_check<T>(err_host, M, error_eps_multiplier, eps);

                // hx_or_b contains A * (calculated X), so res = A * (calculated x) - b = hx_or_b - hb
                ref_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hXorB_1, ldb);
                double err_host_res = matrix_norm_1<T>(M, N, hXorB_1, ldb, hB, M);

                if(arg.unit_check)
                    trsm_err_res_check<T>(err_host_res, M, residual_eps_multiplier, eps);
                err_host = std::max(err_host, err_host_res);
            }
        }

        if(arg.pointer_mode_device)
        {
            // calculate dXorB <- A^(-1) B   rocblas_device_pointer_device
            CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_device));

            // copy hB to hXorB with correct leading dimension as hB still holds input
            copy_matrix_with_different_leading_dimensions(hB, hXorB_1);

            CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));
            CHECK_HIP_ERROR(hipMemcpy(alpha_d, &alpha_h, sizeof(T), hipMemcpyHostToDevice));

            DAPI_CHECK(rocblas_trsm_fn,
                       (handle, side, uplo, transA, diag, M, N, alpha_d, dA, lda, dXorB, ldb));

            CHECK_HIP_ERROR(hXorB_1.transfer_from(dXorB));

            if(arg.repeatability_check)
            {
                HOST_MEMCHECK(host_matrix<T>, hXorB_copy, (M, N, ldb));

                // multi-GPU support
                int device_id, device_count;
                CHECK_HIP_ERROR(hipGetDeviceCount(&device_count));
                for(int dev_id = 0; dev_id < device_count; dev_id++)
                {
                    CHECK_HIP_ERROR(hipGetDevice(&device_id));
                    if(device_id != dev_id)
                        CHECK_HIP_ERROR(hipSetDevice(dev_id));

                    //New rocblas handle for new device
                    rocblas_local_handle handle_copy{arg};

                    //Allocate device memory in new device
                    DEVICE_MEMCHECK(device_matrix<T>, dA_copy, (K, K, lda, HMM));
                    DEVICE_MEMCHECK(device_matrix<T>, dXorB_copy, (M, N, ldb, HMM));
                    DEVICE_MEMCHECK(device_vector<T>, alpha_d_copy, (1));

                    // copy data from CPU to device
                    CHECK_HIP_ERROR(dA_copy.transfer_from(hA));
                    CHECK_HIP_ERROR(
                        hipMemcpy(alpha_d_copy, &alpha_h, sizeof(T), hipMemcpyHostToDevice));

                    CHECK_ROCBLAS_ERROR(
                        rocblas_set_pointer_mode(handle_copy, rocblas_pointer_mode_device));

                    for(int runs = 0; runs < arg.iters; runs++)
                    {
                        copy_matrix_with_different_leading_dimensions(hB, hXorB_copy);
                        CHECK_HIP_ERROR(dXorB_copy.transfer_from(hXorB_copy));
                        CHECK_ROCBLAS_ERROR(rocblas_trsm_fn(handle_copy,
                                                            side,
                                                            uplo,
                                                            transA,
                                                            diag,
                                                            M,
                                                            N,
                                                            alpha_d_copy,
                                                            dA_copy,
                                                            lda,
                                                            dXorB_copy,
                                                            ldb));
                        CHECK_HIP_ERROR(hXorB_copy.transfer_from(dXorB_copy));
                        unit_check_general<T>(M, N, ldb, hXorB_1, hXorB_copy);
                    }
                }
                return;
            }

            if(alpha_h == 0)
            {
                // expecting 0 output, set hX == 0
                rocblas_init_zero((T*)hX, M, N, ldb);

                if(arg.unit_check)
                    unit_check_general<T>(M, N, ldb, hX, hXorB_1);
                if(arg.norm_check)
                    err_device = std::abs(norm_check_general<T>('F', M, N, ldb, hX, hXorB_1));
            }
            else
            {
                err_device = matrix_norm_1<T>(M, N, ldb, hX, hXorB_1);

                if(arg.unit_check)
                    trsm_err_res_check<T>(err_device, M, error_eps_multiplier, eps);

                ref_trmm<T>(side, uplo, transA, diag, M, N, 1.0 / alpha_h, hA, lda, hXorB_1, ldb);
                double err_device_res = matrix_norm_1<T>(M, N, hXorB_1, ldb, hB, M);

                if(arg.unit_check)
                    trsm_err_res_check<T>(err_device_res, M, residual_eps_multiplier, eps);
                err_device = std::max(err_device, err_device_res);
            }
        }
    }

    if(arg.timing)
    {
        double gpu_time_used, cpu_time_used;
        int    number_cold_calls = arg.cold_iters;
        int    total_calls       = number_cold_calls + arg.iters;

        // GPU rocBLAS
        CHECK_HIP_ERROR(dXorB.transfer_from(hXorB_1));
        CHECK_ROCBLAS_ERROR(rocblas_set_pointer_mode(handle, rocblas_pointer_mode_host));

        hipStream_t stream;
        CHECK_ROCBLAS_ERROR(rocblas_get_stream(handle, &stream));

        for(int i = 0; i < total_calls; i++)
        {
            if(i == number_cold_calls)
                gpu_time_used = get_time_us_sync(stream); // in microseconds

            DAPI_DISPATCH(rocblas_trsm_fn,
                          (handle, side, uplo, transA, diag, M, N, &alpha_h, dA, lda, dXorB, ldb));
        }

        gpu_time_used = get_time_us_sync(stream) - gpu_time_used;

        // CPU cblas
        copy_matrix_with_different_leading_dimensions(hB, hXorB_1);
        cpu_time_used = get_time_us_no_sync();

        ref_trsm<T>(side, uplo, transA, diag, M, N, alpha_h, hA, lda, hXorB_1, ldb);

        cpu_time_used = get_time_us_no_sync() - cpu_time_used;

        ArgumentModel<e_side, e_uplo, e_transA, e_diag, e_M, e_N, e_alpha, e_lda, e_ldb>{}
            .log_args<T>(rocblas_cout,
                         arg,
                         gpu_time_used,
                         trsm_gflop_count<T>(M, N, K),
                         ArgumentLogging::NA_value,
                         cpu_time_used,
                         err_host,
                         err_device);
    }
}
