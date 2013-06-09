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
    if ( !mIsEqual(angles->data().get(ofsidx,zidx),val,1e-2) ) return false


bool isAngleOK(PreStack::Gather* angles)
{
    mCheckVal( 0, 0, 0 );
    mCheckVal( 0, 28, 0 );
    mCheckVal( 1, 5, 0.9779 );
    mCheckVal( 1, 25, 0.2413 );
    mCheckVal( 2, 10, 0.9361 );
    mCheckVal( 2, 20, 0.5657 );
    mCheckVal( 3, 10, 1.1124 );
    mCheckVal( 3, 20, 0.7601 );
    mCheckVal( 4, 5, 1.3969 );
    mCheckVal( 4, 25, 0.7764 );
    mCheckVal( 5, 0, 1.5439 );
    mCheckVal( 5, 28, 0.8382 );

    return true;
}


bool BatchProgram::go( std::ostream &strm )
{
    od_init_test_program( GetArgC(), GetArgV() );
    OD::ModDeps().ensureLoaded( "Velocity" );
    RefMan<PreStack::VelocityBasedAngleComputer> computer = 
				    new PreStack::VelocityBasedAngleComputer;

    computer->setMultiID( MultiID(100010,8) );

    if ( !computer->isOK() )
    {
	std::cerr<<" Angle computer is not OK.\n";
	return false;
    }

    StepInterval<double> zrange(0,1.1,0.04), offsetrange(0,2500,500);
    FlatPosData fp;
    fp.setRange( true, offsetrange );
    fp.setRange( false, zrange );
    computer->setOutputSampling( fp );
    computer->setTraceID( BinID(426,800) );

    IOPar iopar;
    iopar.set( PreStack::AngleComputer::sKeySmoothType(), 
	       PreStack::AngleComputer::TimeAverage );
    iopar.set( PreStack::AngleComputer::sKeyWinFunc(), HanningWindow::sName() );
    iopar.set( PreStack::AngleComputer::sKeyWinParam(), 0.95f );
    iopar.set( PreStack::AngleComputer::sKeyWinLen(), 10 );

    computer->setSmoothingPars( iopar );

    PtrMan<PreStack::Gather> angles = computer->computeAngles();
    if ( !angles )
    {
	std::cerr << "Computer did not succeed in making angle data\n";
	return false;
    }

    if ( !isAngleOK(angles) )
    {
	std::cerr << "Computer computed wrong values\n";
	return false;
    }

    return true;
}

