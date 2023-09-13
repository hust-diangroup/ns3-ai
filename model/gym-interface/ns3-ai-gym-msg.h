/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#ifndef NS3_NS3_AI_GYM_MSG_H
#define NS3_NS3_AI_GYM_MSG_H

#include <stdint.h>

#define MSG_BUFFER_SIZE 1024

struct Ns3AiGymMsg
{
    uint8_t buffer[MSG_BUFFER_SIZE];
    uint32_t size;
};

#endif // NS3_NS3_AI_GYM_MSG_H
