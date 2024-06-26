/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef _rt_fixed_public
#define _rt_fixed_public

#include <stdint.h>

/* base type */
typedef int32_t fixed;

/* int & float to fixed */
#define FIXED(a) ((fixed)((a) * (1 << 16)))

/* fixed to int */
#define FIXED_TO_INT(a) ((int)((a) >> 16))

/* fixed to float */
#define FIXED_TO_FLOAT(a) ((float)((a) * (1.0f / (float)(1 << 16))))

/* math functions */
fixed FixedMul(fixed a, fixed b);
fixed FixedDiv2(fixed a, fixed b);
fixed FixedScale(fixed orig, fixed factor, fixed divisor);
fixed FixedMulShift(fixed a, fixed b, fixed shift);
fixed FixedSqrt(fixed n);

#endif
