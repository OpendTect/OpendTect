/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID mUsedVar = "$Id$"; 

#include "batchprog.h"

#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "windowfunction.h"
#include <iostream>


#define mCheckVal(ofsidx,zidx,val) \
    if ( !mIsEqual(angles->data().get(ofsidx,zidx),val,1e-6f) ) return false


bool isRawAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0.f );
    mCheckVal( 5, 0, 1.5707964f );
    mCheckVal( 1, 50, 0.90732318f );
    mCheckVal( 4, 50, 1.377309f );
    mCheckVal( 2, 100, 0.90647662f );
    mCheckVal( 3, 100, 1.0894637f );
    mCheckVal( 2, 200, 0.55024701f );
    mCheckVal( 3, 200, 0.74381065f );
    mCheckVal( 1, 250, 0.23507828f );
    mCheckVal( 4, 250, 0.76394761f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.8210543f );

    return true;
}


bool isMovingAverageAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0.f );
    mCheckVal( 5, 0, 1.5605127f );
    mCheckVal( 1, 50, 0.90902019f );
    mCheckVal( 4, 50, 1.3773699f );
    mCheckVal( 2, 100, 0.90667629f );
    mCheckVal( 3, 100, 1.0895568f );
    mCheckVal( 2, 200, 0.55048656f );
    mCheckVal( 3, 200, 0.74400896f );
    mCheckVal( 1, 250, 0.23520035f );
    mCheckVal( 4, 250, 0.76409501f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.82791579f );

    return true;
}


bool isFFTAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0.f );
    mCheckVal( 5, 0, 1.5611646f );
    mCheckVal( 1, 50, 0.91011989f );
    mCheckVal( 4, 50, 1.3775427f );
    mCheckVal( 2, 100, 0.90640467f );
    mCheckVal( 3, 100, 1.0894272f );
    mCheckVal( 2, 200, 0.55032426f );
    mCheckVal( 3, 200, 0.74386501f );
    mCheckVal( 1, 250, 0.23386876f );
    mCheckVal( 4, 250, 0.76376468f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.80836409f );

    return true;
}


bool BatchProgram::go( od_ostream& strm )
{
    od_init_test_program( GetArgC(), GetArgV() );
    OD::ModDeps().ensureLoaded( "Velocity" );
    RefMan<PreStack::VelocityBasedAngleComputer> computer = 
				    new PreStack::VelocityBasedAngleComputer;

    PtrMan<IOObj> velobj = IOM().get( MultiID("100010.8") );
    if ( !velobj )
    {
	std::cerr<<" Input data is not available.\n";
	return false;
    }

    computer->setMultiID( velobj->key() );
    StepInterval<double> zrange(0,1.1,0.004), offsetrange(0,2500,500);
    FlatPosData fp;
    fp.setRange( true, offsetrange );
    fp.setRange( false, zrange );
    computer->setOutputSampling( fp );
    computer->setTraceID( BinID(426,800) );
    if ( !computer->isOK() )
    {
	std::cerr<<" Angle computer is not OK.\n";
	return false;
    }

    PtrMan<PreStack::Gather> angles = computer->computeAngles();
    if ( !angles )
    {
	std::cerr << "Computer did not succeed in making angle data\n";
	return false;
    }

    if ( !isRawAngleOK(angles) )
    {
	std::cerr << "Angle computer computed wrong raw values\n";
	return false;
    }

    computer->setMovingAverageSmoother( 0.1f, HanningWindow::sName() );
    angles = computer->computeAngles();
    if ( !isMovingAverageAngleOK(angles) )
    {
	std::cerr << "Angle computer computed wrong values after AVG filter\n";
	return false;
    }

    computer->setFFTSmoother( 10.f, 15.f );
    angles = computer->computeAngles();
    if ( !isFFTAngleOK(angles) )
    {
	std::cerr << "Angle computer computed wrong values after FFT filter\n";
	return false;
    }

    return true;
}

