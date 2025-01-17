/****************************************************************************
 *
 * fttypes.h
 *
 *   FreeType simple types definitions (specification only).
 *
 * Copyright (C) 1996-2020 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, FTL.txt.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */

#ifndef PLUTOVG_FT_TYPES_H
#define PLUTOVG_FT_TYPES_H

/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Fixed                                                           */
/*                                                                       */
/* <Description>                                                         */
/*    This type is used to store 16.16 fixed-point values, like scaling  */
/*    values or matrix coefficients.                                     */
/*                                                                       */
typedef signed long  vector_fixed_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Int                                                             */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef for the int type.                                        */
/*                                                                       */
typedef signed int  vector_int_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_UInt                                                            */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef for the unsigned int type.                               */
/*                                                                       */
typedef unsigned int  vector_uint_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Long                                                            */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef for signed long.                                         */
/*                                                                       */
typedef signed long  vector_long_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_ULong                                                           */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef for unsigned long.                                       */
/*                                                                       */
typedef unsigned long vector_ulong_t;

/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Short                                                           */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef for signed short.                                        */
/*                                                                       */
typedef signed short  vector_short_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Byte                                                            */
/*                                                                       */
/* <Description>                                                         */
/*    A simple typedef for the _unsigned_ char type.                     */
/*                                                                       */
typedef unsigned char  vector_byte_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Bool                                                            */
/*                                                                       */
/* <Description>                                                         */
/*    A typedef of unsigned char, used for simple booleans.  As usual,   */
/*    values 1 and~0 represent true and false, respectively.             */
/*                                                                       */
typedef unsigned char  vector_bool_t;



/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Error                                                           */
/*                                                                       */
/* <Description>                                                         */
/*    The FreeType error code type.  A value of~0 is always interpreted  */
/*    as a successful operation.                                         */
/*                                                                       */
typedef int  vector_error_t;


/*************************************************************************/
/*                                                                       */
/* <Type>                                                                */
/*    PVG_FT_Pos                                                             */
/*                                                                       */
/* <Description>                                                         */
/*    The type PVG_FT_Pos is used to store vectorial coordinates.  Depending */
/*    on the context, these can represent distances in integer font      */
/*    units, or 16.16, or 26.6 fixed-point pixel coordinates.            */
/*                                                                       */
typedef signed long  vector_pos_t;


/*************************************************************************/
/*                                                                       */
/* <Struct>                                                              */
/*    PVG_FT_Vector                                                          */
/*                                                                       */
/* <Description>                                                         */
/*    A simple structure used to store a 2D vector; coordinates are of   */
/*    the PVG_FT_Pos type.                                                   */
/*                                                                       */
/* <Fields>                                                              */
/*    x :: The horizontal coordinate.                                    */
/*    y :: The vertical coordinate.                                      */
/*                                                                       */
typedef struct  vector_
{
    vector_pos_t  x;
    vector_pos_t  y;

} vector_t;


typedef long long int           vector_int64_t;
typedef unsigned long long int  vector_uint64_t;

typedef signed int              vector_int32_t;
typedef unsigned int            vector_uint32_t;

#define PVG_FT_BOOL( x )  ( (vector_bool_t)( x ) )

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#endif // PLUTOVG_FT_TYPES_H
