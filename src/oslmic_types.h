/*

Module:  oslmic_types.h

Function:
        Basic types from oslmic.h, shared by all layers.

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       November 2018
        (based on oslmic.h from IBM).

*/

#ifndef _oslmic_types_h_
# define _oslmic_types_h_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//================================================================================
//================================================================================
// Target platform as C library
typedef uint8_t            bit_t;
typedef uint8_t            u1_t;
typedef int8_t             s1_t;
typedef uint16_t           u2_t;
typedef int16_t            s2_t;
typedef uint32_t           u4_t;
typedef int32_t            s4_t;
typedef unsigned int       uint;
typedef const char*        str_t;

// the HAL needs to give us ticks, so it ought to know the right type.
typedef              s4_t  ostime_t;

typedef     struct osjob_t osjob_t;
//typedef      struct band_t band_t;
typedef   struct chnldef_t chnldef_t;
typedef   struct rxsched_t rxsched_t;
typedef   struct bcninfo_t bcninfo_t;
typedef        const u1_t* xref2cu1_t;
typedef              u1_t* xref2u1_t;
#ifdef __cplusplus
}
#endif

/* end of oslmic_types.h */
#endif /* _oslmic_types_h_ */
