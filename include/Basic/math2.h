#ifndef math2_H
#define math2_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: math2.h,v 1.7 2008-07-16 17:59:59 cvsnanne Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

/* Functions with some extra facilities added to math.h

   Each function has a float and a double version. The float version is
   prefixed with 'f', the double with 'd'. This seems very old-fashioned, but
   there are two reasons:
   1) The compiler will complain rather than choose one in many circumstances
   2) It forces thought about the matter

 */

namespace Math
{

bool		IsNormalNumber(float);
		/* Returns 0 for for infinite, NaN, and that sort of crap */
float		IntPowerOf(float,int);
float		PowerOf(float,float);
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */
float		ASin(float);
		/*!<Checks the input range before calling asin, and does thus
		    avoid nan's due to roundoff errors. */
float		ACos(float);
		/*!<Checks the input range before calling acos, and does thus
		    avoid nan's due to roundoff errors. */
float		Log(float);
                /*!<Checks the input range before calling log, returns
		    undefined if negative or zero value is given. */
float		Log10(float);
                /*!<Checks the input range before calling log10, returns
		    mUdf(float) if negative or zero value is given. */
float		Sqrt(float);
                /*!<Checks the input range before calling sqrt, if negative
		    value is given, zero is returned. */
float		Exp(float);
                /*!<Checks the input range before calling exp, if too large
		    value is given, mUdf(float) is returned. */

inline float	degFromNorth( float azimuth )
		{
		    const float deg = 90 - 57.2957795131f * azimuth;
		    return deg < 0 ? deg + 360 : deg;
		}

int		LCMOf(int,int);
bool		IsNormalNumber(double);
double		IntPowerOf(double,int);
double		PowerOf(double,double);
double		ASin(double);
double		ACos(double);
double		Log(double);
double		Log10(double);
double		Exp(double);
double		Sqrt(double);

} // namespace Math


#endif
