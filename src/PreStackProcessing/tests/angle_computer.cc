/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"
#include "testprog.h"

#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "windowfunction.h"


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


#define mCompVal(ofsidx,zidx,val) \
    if ( !mIsEqual(angles->data().get(ofsidx,zidx),val,1e-1f) ) return false

bool compareAngles(PreStack::Gather* angles,int zidx)
{
    const int lastidx = angles->data().info().getSize( 1 ) - 1;
    const int mididx = lastidx / 2;

    mCompVal( 0, 0, 0 );
    mCompVal( 1, 0, 1.57f );
    mCompVal( 2, 0, 1.57f );
    mCompVal( 3, 0, 1.57f );
    mCompVal( 4, 0, 1.57f );
    mCompVal( 5, 0, 1.57f );

    mCompVal( 0, mididx, 0 );
    mCompVal( 1, mididx, 0.40f );
    mCompVal( 2, mididx, 0.70f );
    mCompVal( 3, mididx, 0.90f );
    mCompVal( 4, mididx, 1.05f );
    mCompVal( 5, mididx, 1.15f );

    mCompVal( 0, zidx, 0 );
    mCompVal( 1, zidx, 0.20f );
    mCompVal( 2, zidx, 0.40f );
    mCompVal( 3, zidx, 0.55f );
    mCompVal( 4, zidx, 0.70f );
    mCompVal( 5, zidx, 0.80f );

    mCompVal( 0, lastidx, 0 );
    mCompVal( 1, lastidx, 0.20f );
    mCompVal( 2, lastidx, 0.40f );
    mCompVal( 3, lastidx, 0.55f );
    mCompVal( 4, lastidx, 0.70f );
    mCompVal( 5, lastidx, 0.80f );

    return true;
}



bool testAnglesForDifferentSurveys();


bool BatchProgram::go( od_ostream& strm )
{
    mInitBatchTestProg();

    OD::ModDeps().ensureLoaded( "Velocity" );
    RefMan<PreStack::VelocityBasedAngleComputer> computer =
				    new PreStack::VelocityBasedAngleComputer;

    PtrMan<IOObj> velobj = IOM().get( MultiID("100010.8") );
    if ( !velobj )
    {
	od_cout() << " Input data is not available.\n";
	return false;
    }

    computer->setMultiID( velobj->key() );
    StepInterval<double> zrange(0,1.1,0.004), offsetrange(0,2500,500);
    FlatPosData fp;
    fp.setRange( true, offsetrange );
    fp.setRange( false, zrange );
    computer->setOutputSampling( fp );
    computer->setTrcKey( TrcKey(BinID(426,800)) );
    if ( !computer->isOK() )
    {
	od_cout() << " Angle computer is not OK.\n";
	return false;
    }

    PtrMan<PreStack::Gather> angles = computer->computeAngles();
    if ( !angles )
    {
	od_cout() << "Computer did not succeed in making angle data\n";
	return false;
    }

    if ( !isRawAngleOK(angles) )
    {
	od_cout() << "Angle computer computed wrong raw values\n";
	return false;
    }

    computer->setMovingAverageSmoother( 0.1f, HanningWindow::sName() );
    angles = computer->computeAngles();
    if ( !isMovingAverageAngleOK(angles) )
    {
	od_cout() << "Angle computer computed wrong values after AVG filter\n";
	return false;
    }

    computer->setFFTSmoother( 10.f, 15.f );
    angles = computer->computeAngles();
    if ( !isFFTAngleOK(angles) )
    {
	od_cout() << "Angle computer computed wrong values after FFT filter\n";
	return false;
    }

    if ( !testAnglesForDifferentSurveys() )
    {
	od_cout() << "Failed while comparing values in different surveys\n";
	return false;
    }

    return true;
}


bool testAnglesForDifferentSurveys()
{
    const int nrsurveys = 6;
    const char* survnames[] = { "F3_Test_Survey", "F3_Test_Survey_DepthFT", 
				"F3_Test_Survey_DepthFT__XYinft_", 
				"F3_Test_Survey_DepthM",
				"F3_Test_Survey_DepthM_XYinft", 
				"F3_Test_Survey_XYinft" };

    TypeSet<StepInterval<double> > zrgs;
    zrgs.add( StepInterval<double> (0,1.1,0.004) );
    zrgs.add( StepInterval<double> (0,3930,15) );
    zrgs.add( StepInterval<double> (0,3930,15) );
    zrgs.add( StepInterval<double> (0,1200,5) );
    zrgs.add( StepInterval<double> (0,1200,5) );
    zrgs.add( StepInterval<double> (0,1.1,0.004) );

    const int zids [] = { 256, 257, 257, 235, 235, 256 };

    for ( int idx = 0; idx < nrsurveys; idx++ )
    {
	IOM().setSurvey( survnames[idx] );
	RefMan<PreStack::VelocityBasedAngleComputer> computer =
				new PreStack::VelocityBasedAngleComputer;

	PtrMan<IOObj> velobj = IOM().get( MultiID("100010.8") );
	if ( !velobj )
	{
	    od_cout() << survnames[idx];
	    od_cout() << " : Input data is not available.\n";
	    return false;
	}

	computer->setMultiID( velobj->key() );
	StepInterval<double> zrange(zrgs[idx]), offsetrange(0,2500,500);
	FlatPosData fp;
	fp.setRange( true, offsetrange );
	fp.setRange( false, zrange );
	computer->setOutputSampling( fp );
	computer->setTrcKey( TrcKey(BinID(426,950)) );
	if ( !computer->isOK() )
	{
	    od_cout() << survnames[idx];
	    od_cout() << " : Angle computer is not OK.\n";
	    return false;
	}

	PtrMan<PreStack::Gather> angles = computer->computeAngles();
	if ( !angles )
	{
	    od_cout() << survnames[idx];
	    od_cout() << " : Computer did not succeed in making angle data\n";
	    return false;
	}

	for ( int ofsidx = 0; ofsidx <= offsetrange.nrSteps(); ofsidx++ )
	{
	    TypeSet<float> anglevals;
	    for ( int zidx = 0; zidx <= zrgs[idx].nrSteps(); zidx++ )
	    {
		float angle = angles->data().get( ofsidx, zidx );
		anglevals += angle;
	    }
	}
	
	if ( !compareAngles(angles,zids[idx]) )
	{
	    od_cout() << survnames[idx];
	    od_cout() << " : Angle computer computed wrong values\n";
	    return false;
	}
    }

    return true;
}

