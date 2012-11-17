#ifndef math2_h
#define math2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include <complex>

/* Functions with some extra facilities added to math.h

   Each function has a float and a double version. This may be very
   old-fashioned, but who cares.

   At the end, there is also an all-integer x to the power y.

 */

typedef std::complex<float> float_complex;

namespace Math
{
    
mGlobal(Basic) unsigned int SetFlags( unsigned int curflags,
                              unsigned int flag, bool yn );
    		/*!<Takes curflags, sets/clears the bits in flag, and returns
                    the composite value.*/
mGlobal(Basic) bool IsNormalNumber(float);
		/* Returns 0 for for infinite, NaN, and that sort of crap */
mGlobal(Basic) float IntPowerOf(float,int);
mGlobal(Basic) float PowerOf(float,float);
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */
mGlobal(Basic) float ASin(float);
		/*!<Checks the input range before calling asin, and does thus
		    avoid nan's due to roundoff errors. */
mGlobal(Basic) float ACos(float);
		/*!<Checks the input range before calling acos, and does thus
		    avoid nan's due to roundoff errors. */
mGlobal(Basic) float Log(float);
                /*!<Checks the input range before calling log, returns
		    undefined if negative or zero value is given. */
mGlobal(Basic) float Log10(float);
                /*!<Checks the input range before calling log10, returns
		    mUdf(float) if negative or zero value is given. */
mGlobal(Basic) float Sqrt(float);
                /*!<Checks the input range before calling sqrt, if negative
		    value is given, zero is returned. */
mGlobal(Basic) float_complex Sqrt(const float_complex&);
mGlobal(Basic) float Exp(float);
                /*!<Checks the input range before calling exp, if too large
		    value is given, mUdf(float) is returned. */
mGlobal(Basic) float toDB(float);

inline float	degFromNorth( float azimuth )
		{
		    const float deg = 90 - 57.2957795131f * azimuth;
		    return deg < 0 ? deg + 360 : deg;
		}

mGlobal(Basic) int LCMOf(int,int); /*! <Lowest Common Multiple. */
mGlobal(Basic) int HCFOf(int,int); /*! <Highest Common Factor. */
mGlobal(Basic) bool IsNormalNumber(double);
mGlobal(Basic) double IntPowerOf(double,int);
mGlobal(Basic) double PowerOf(double,double);
mGlobal(Basic) double ASin(double);
mGlobal(Basic) double ACos(double);
mGlobal(Basic) double Log(double);
mGlobal(Basic) double Log10(double);
mGlobal(Basic) double Exp(double);
mGlobal(Basic) double Sqrt(double);
mGlobal(Basic) double toDB(double);

mGlobal(Basic) int IntPowerOf(int,int);
mGlobal(Basic) od_int64 IntPowerOf(od_int64,int);

} // namespace Math


#endif

