#ifndef math2_H
#define math2_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: math2.h,v 1.8 2008-12-18 05:23:26 cvsranojay Exp $
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

mGlobal( bool IsNormalNumber(float) )
		/* Returns 0 for for infinite, NaN, and that sort of crap */
mGlobal( float IntPowerOf(float,int) )
mGlobal( float PowerOf(float,float) )
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */
mGlobal( float ASin(float) )
		/*!<Checks the input range before calling asin, and does thus
		    avoid nan's due to roundoff errors. */
mGlobal( float ACos(float) )
		/*!<Checks the input range before calling acos, and does thus
		    avoid nan's due to roundoff errors. */
mGlobal( float Log(float) )
                /*!<Checks the input range before calling log, returns
		    undefined if negative or zero value is given. */
mGlobal( float Log10(float) )
                /*!<Checks the input range before calling log10, returns
		    mUdf(float) if negative or zero value is given. */
mGlobal( float Sqrt(float) )
                /*!<Checks the input range before calling sqrt, if negative
		    value is given, zero is returned. */
mGlobal( float Exp(float) )
                /*!<Checks the input range before calling exp, if too large
		    value is given, mUdf(float) is returned. */

inline float	degFromNorth( float azimuth )
		{
		    const float deg = 90 - 57.2957795131f * azimuth;
		    return deg < 0 ? deg + 360 : deg;
		}

mGlobal(int LCMOf(int,int))
mGlobal(bool IsNormalNumber(double))
mGlobal(double IntPowerOf(double,int))
mGlobal(double PowerOf(double,double))
mGlobal(double ASin(double))
mGlobal(double ACos(double))
mGlobal(double Log(double))
mGlobal(double Log10(double))
mGlobal(double Exp(double))
mGlobal(double Sqrt(double))

} // namespace Math


#endif
