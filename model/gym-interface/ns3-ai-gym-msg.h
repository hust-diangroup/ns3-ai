/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 * Copyright (c) 2026 Muhammad Uzair (modernization)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Author: Muyuan Shen <muyuan_shen@hust.edu.cn>
 *
 * Changes:
 *   - Increased default MSG_BUFFER_SIZE from 1024 to 8192 to support
 *     large observation/action spaces (e.g., multi-agent, image-based)
 *   - Made configurable via NS3_AI_MSG_BUFFER_SIZE define
 *   - Added compile-time size validation
 */

#ifndef NS3_NS3_AI_GYM_MSG_H
#define NS3_NS3_AI_GYM_MSG_H

#include <cstdint>

/**
 * \brief Size of the protobuf message buffer in bytes.
 *
 * Override at compile time with -DNS3_AI_MSG_BUFFER_SIZE=<size>
 * Default: 8192 bytes (supports observation spaces up to ~8KB)
 *
 * For large observation spaces (e.g., images, multi-agent with many UEs),
 * increase this value. The buffer is allocated in shared memory.
 */
#ifndef NS3_AI_MSG_BUFFER_SIZE
#define NS3_AI_MSG_BUFFER_SIZE 8192
#endif

#define MSG_BUFFER_SIZE NS3_AI_MSG_BUFFER_SIZE

static_assert(MSG_BUFFER_SIZE >= 256, "MSG_BUFFER_SIZE must be at least 256 bytes");
static_assert(MSG_BUFFER_SIZE <= 1048576, "MSG_BUFFER_SIZE should not exceed 1MB");

/**
 * \brief Shared memory message for gym interface
 *
 * Contains a serialized protobuf message (observation or action)
 * and its actual size. The buffer is fixed-size for shared memory
 * compatibility (no dynamic allocation in shared memory).
 */
struct Ns3AiGymMsg
{
    uint8_t buffer[MSG_BUFFER_SIZE]; //!< Serialized protobuf data
    uint32_t size;                   //!< Actual message size in bytes
};

#endif // NS3_NS3_AI_GYM_MSG_H
