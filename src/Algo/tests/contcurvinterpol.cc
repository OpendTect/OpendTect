
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : DZH
 * DATE     : April 2015
 * FUNCTION :
-*/



#include "arraynd.h"
#include "arrayndimpl.h"
#include "thread.h"
#include "atomic.h"
#include "testprog.h"
#include "contcurvinterpol.h"

#define epsilon 1e-5  

const Array2D<float>* getFixedGMTInterpolatedResult()
{
   /* below data are created by GMT surface and grd2xyz applications.
   surface command: -- gmtxyz.txt -R0/5/0/5 -I1 -GFgmtgrd.grd -T0.25 -V1
   gmtxyz -- is input asccii file which contains the exact same data in 
   function createArray2DData().
   -R0/5/0/5 --grid range( row and col )
   gmtgrd.grd -- out put gmt grd file
   -T0.25 - tension parameter
   -V1 - report iteration information. not important
   grd2xyz command: gmtgrd.grd > gmtout.txt
   gmtgrd.grd -- created by surface application.
   gmtout.txt -- output ascii file. the data in this file are hard codede in
   below.
   */

   Array2DImpl<float>* arr = new Array2DImpl<float>( 6, 6 );

    arr->set( 0 ,0, 1.09602999687 );
    arr->set( 1, 0, 1.05400550365 );
    arr->set( 2 ,0, 1.00562000275 );
    arr->set( 3, 0, 0.942700028419 );
    arr->set( 4, 0, 0.907797396183 );
    arr->set( 5, 0, 0.869086444378 );
    arr->set( 0, 1, 1.07421994209 );
    arr->set( 1, 1, 1.04504668713 );
    arr->set( 2, 1, 1.00725495815 );
    arr->set( 3, 1, 0.967710018158 );
    arr->set( 4, 1, 0.898368597031 );
    arr->set( 5, 1, 0.841876804829 );
    arr->set( 0, 2, 1.05322003365 );
    arr->set( 1, 2, 1.03905928135 );
    arr->set( 2, 2, 1.00509095192 );
    arr->set( 3, 2, 0.904551029205 );
    arr->set( 4, 2, 0.852483451366 );
    arr->set( 5, 2, 0.801722347736 );
    arr->set( 0, 3, 1.04480004311 );
    arr->set( 1, 3, 1.03523182869 );
    arr->set( 2, 3, 1.00427401066 );
    arr->set( 3, 3, 0.903828024864 );
    arr->set( 4, 3, 0.833339631557 );
    arr->set( 5, 3, 0.77356916666 );
    arr->set( 0, 4, 1.02664995193 );
    arr->set( 1, 4, 1.02655434608 );
    arr->set( 2, 4, 1.00247395039 );
    arr->set( 3, 4, 0.902226984501 );
    arr->set( 4, 4, 0.823357760906 );
    arr->set( 5, 4, 0.755713105202 );
    arr->set( 0, 5, 1.0287989378 );
    arr->set( 1, 5, 1.01663243771 );
    arr->set( 2, 5, 0.983038723469 );
    arr->set( 3, 5, 0.898606181145 );
    arr->set( 4, 5, 0.815937101841 );
    arr->set( 5, 5, 0.742695510387 );

    return arr;
}


Array2D<float>* createArray2DData()
{
    Array2DImpl<float>* arr = new Array2DImpl<float>( 6, 6 );

    arr->setAll( mUdf(float) );

    arr->set( 0, 0, 1.09603 );
    arr->set( 0, 1, 1.07422 );
    arr->set( 0, 2, 1.05322 );
    arr->set( 0, 3, 1.0448 );
    arr->set( 0, 4, 1.02665 );

    arr->set( 2, 0, 1.00562 );
    arr->set( 2, 1, 1.007255 );
    arr->set( 2, 2, 1.005091 );
    arr->set( 2, 3, 1.004274 );
    arr->set( 2, 4, 1.002474 );

    arr->set( 3, 0, 0.9427 );
    arr->set( 3, 1, 0.96771 );
    arr->set( 3, 2, 0.904551 );
    arr->set( 3, 3, 0.903828 );
    arr->set( 3, 4, 0.902227 );
   
    return arr;
}


bool testContinuousCurvatureInterpolation()
{
    PtrMan<Array2D<float> > arr = createArray2DData();

    mDeclareAndTryAlloc( PtrMan<ContinuousCurvatureArray2DInterpol>, interpol, 
	ContinuousCurvatureArray2DInterpol() );
    interpol->setArray( *arr, 0 );

    mRunStandardTest( interpol->execute(), "Run intererpolation" );
    
    ConstPtrMan<Array2D<float> > gmtarr = getFixedGMTInterpolatedResult();
    const int rows = gmtarr->info().getSize(0);
    const int cols = gmtarr->info().getSize(1);

    mRunStandardTest( rows>0 && cols>0, " Result has valid size" );

    double diffsum = .0;
    for ( int idy=0; idy<cols; idy++ )
    {
	for ( int idx=0; idx<rows; idx++ )
	{
	    if ( mIsUdf(arr->get(idx,idy)) || gmtarr->get(idx,idy) ==0 )
		return false;
	    diffsum += arr->get(idx,idy) - gmtarr->get( idx, idy );
	}
    }

    mRunStandardTest( diffsum<epsilon, " Interpolation has correct values" );
 
    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
 
    return testContinuousCurvatureInterpolation() ? 0 : 1;

}
