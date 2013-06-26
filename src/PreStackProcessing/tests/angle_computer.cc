/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID mUsedVar = "$Id$"; 

#include "batchprog.h"

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
    mCheckVal( 5, 0, 1.570796f );
    mCheckVal( 1, 50, 0.9078106f );
    mCheckVal( 4, 50, 1.377318f );
    mCheckVal( 2, 100, 0.9066244f );
    mCheckVal( 3, 100, 1.089529f );
    mCheckVal( 2, 200, 0.5504489f );
    mCheckVal( 3, 200, 0.7439767f );
    mCheckVal( 1, 250, 0.2351587f );
    mCheckVal( 4, 250, 0.7640429f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.8210543f );

    return true;
}


bool isMovingAverageAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0.f );
    mCheckVal( 5, 0, 1.560514f );
    mCheckVal( 1, 50, 0.9093758f );
    mCheckVal( 4, 50, 1.377381f );
    mCheckVal( 2, 100, 0.9067743f );
    mCheckVal( 3, 100, 1.089602f );
    mCheckVal( 2, 200, 0.5505398f );
    mCheckVal( 3, 200, 0.7440532f );
    mCheckVal( 1, 250, 0.2352430f );
    mCheckVal( 4, 250, 0.7641466f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.8279510f );

    return true;
}


bool isFFTAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0.f );
    mCheckVal( 5, 0, 1.561165f );
    mCheckVal( 1, 50, 0.910470f );
    mCheckVal( 4, 50, 1.377556f );
    mCheckVal( 2, 100, 0.906494f );
    mCheckVal( 3, 100, 1.089470f );
    mCheckVal( 2, 200, 0.550383f );
    mCheckVal( 3, 200, 0.743913f );
    mCheckVal( 1, 250, 0.233914f );
    mCheckVal( 4, 250, 0.763819f );
    mCheckVal( 0, 275, 0.f );
    mCheckVal( 5, 275, 0.808400f );

    return true;
}


bool BatchProgram::go( std::ostream &strm )
{
    od_init_test_program( GetArgC(), GetArgV() );
    OD::ModDeps().ensureLoaded( "Velocity" );
    RefMan<PreStack::VelocityBasedAngleComputer> computer = 
				    new PreStack::VelocityBasedAngleComputer;

    computer->setMultiID( MultiID(100010,8) );
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

