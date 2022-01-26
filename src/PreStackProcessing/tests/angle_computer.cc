/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/


#include "batchprog.h"
#include "testprog.h"

#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "windowfunction.h"

/*
   The test program is based on the following synthetic model:
   Layer 1: H=561.5498m, Vp = 1960m/s
   Layer 2: H=54.81m,	 Vp = 2175m/s
   Layer 3: H=379.228m,  Vp = 2260m/s
   Layer 4: H=169.188m,  Vp = 2400m/s
   The interfaces thus cross the interpretations FS8, FS7, FS4 at respectively:
   TWT=573.01, 623.41 and 959.01ms for BinID 426/800
   That model is digitized in the various test surveys exactly for that BinID.

*/

// 1e-3f radians < 0.058 degree
#define mCheckVal(ofsidx,zidx,val,act) \
{ \
    if ( !mIsEqual(angles.data().get(ofsidx,zidx),val,1e-3f) ) \
    { \
	od_cout() << "Failure for offset idx " << ofsidx; \
	od_cout() << " and zidx: " << zidx << od_newline; \
	act; \
	return false; \
    } \
}


bool isRawAngleOK( const PreStack::Gather& angles )
{
    mCheckVal( 0, 0,   0.f,; )
    mCheckVal( 5, 0,   M_PI_2f,; )
    mCheckVal( 1, 50,  0.905887909f,; )
    mCheckVal( 4, 50,  1.377249863f,; )
    mCheckVal( 2, 100, 0.905887909f,; )
    mCheckVal( 3, 100, 1.08918025f,; )
    mCheckVal( 2, 200, 0.549786276f,; )
    mCheckVal( 3, 200, 0.743334588f,; )
    mCheckVal( 1, 250, 0.234869558f,; )
    mCheckVal( 4, 250, 0.763504006f,; )
    mCheckVal( 0, 275, 0.f,; )
    mCheckVal( 5, 275, 0.820676323f,; )

    return true;
}


bool isMovingAverageAngleOK( const PreStack::Gather& angles )
{
    mCheckVal( 0, 0,   0.f,; );
    mCheckVal( 5, 0,   1.55975366f,; );
    mCheckVal( 1, 50,  0.90728283f,; );
    mCheckVal( 4, 50,  1.37730265f,; );
    mCheckVal( 2, 100, 0.90623587f,; );
    mCheckVal( 3, 100, 1.08934581f,; );
    mCheckVal( 2, 200, 0.54997629f,; );
    mCheckVal( 3, 200, 0.7434904f,; );
    mCheckVal( 1, 250, 0.2349564f,; );
    mCheckVal( 4, 250, 0.7636112f,; );
    mCheckVal( 0, 275, 0.f,; );
    mCheckVal( 5, 275, 0.82726491f,; );

    return true;
}


bool isFFTAngleOK( const PreStack::Gather& angles )
{
    mCheckVal( 0, 0,   0.f,; )
    mCheckVal( 5, 0,   1.5606885f,; )
    mCheckVal( 1, 50,  0.9083171f,; )
    mCheckVal( 4, 50,  1.37746108f,; )
    mCheckVal( 2, 100, 0.90595824f,; )
    mCheckVal( 3, 100, 1.08921146f,; )
    mCheckVal( 2, 200, 0.54981774f,; )
    mCheckVal( 3, 200, 0.74334842f,; )
    mCheckVal( 1, 250, 0.23362018f,; )
    mCheckVal( 4, 250, 0.76326013f,; )
    mCheckVal( 0, 275, 0.f,; )
    mCheckVal( 5, 275, 0.80776274f,; )

    return true;
}

// Analytically computed values, not an approximation
static const float anglezeroidx[] = { 0.f, M_PI_2f, M_PI_2f,
					M_PI_2f, M_PI_2f, M_PI_2f };
static const float anglestimesurvlastidx[] = { 0.f, 0.211425903f, 0.405479412f,
					0.572076f, 0.70942786f, 0.82067632f };
static const float anglestimesurvmididx[] = { 0.f, 0.43568076f, 0.7496963f,
					0.9493774f, 1.0779576f, 1.1650088847f };
static const float anglestimesurvzidx[] = { 0.f, 0.25347754f, 0.47802176f,
					0.66065097f, 0.80317355f, 0.91332992f };
static const float anglesdepthmsurvlastidx[] = { 0.f, 0.20539539f, 0.39479112f,
					0.5585993f, 0.69473828f, 0.8058035f };
static const float anglesdepthmsurvmididx[] = { 0.f, 0.3947911f, 0.69473827f,
					0.89605539f, 1.03037683f, 1.12327635f };
static const float anglesdepthmsurvzidx[] = { 0.f, 0.25349427f, 0.478049905f,
					0.66068436f, 0.803208f, 0.913363251f };
static const float anglesdepthftsurvlastidx[] = { 0.f, 0.2057514f, 0.39542403f,
					0.5594f, 0.6956147f, 0.806693513f };
static const float anglesdepthftsurvmididx[] = { 0.f, 0.39542403f, 0.69561461f,
					0.89692428f, 1.03116249f, 1.123971f };
static const float anglesdepthftsurvzidx[] = { 0.f, 0.2535718f, 0.47818031f,
					0.660839038f, 0.8033675f, 0.91351766f };


bool compareAngles( const PreStack::Gather& angles, int zidx, bool depth,
		    bool feet )
{
    const int nroffset = angles.data().info().getSize( 0 );
    TypeSet<int> zidxs;
    zidxs += 0;
    zidxs += angles.data().info().getSize( 1 ) - 1;
    zidxs += zidxs[1]/2;
    zidxs += zidx;
    const int nrztests = zidxs.size();

    mDeclareAndTryAlloc(const float**,targetvals,const float*[nrztests])
    if ( !targetvals )
	return false;

    targetvals[0] = anglezeroidx;
    targetvals[1] = depth
		  ? (feet ? anglesdepthftsurvlastidx : anglesdepthmsurvlastidx)
		  : anglestimesurvlastidx;
    targetvals[2] = depth
		  ? (feet ? anglesdepthftsurvmididx : anglesdepthmsurvmididx)
		  : anglestimesurvmididx;
    targetvals[3] = depth
		  ? (feet ? anglesdepthftsurvzidx : anglesdepthmsurvzidx )
		  : anglestimesurvzidx;
    for ( int idx=0; idx<nrztests; idx++ )
    {
	for ( int ioff=0; ioff<nroffset; ioff++ )
	{
	    mCheckVal( ioff, zidxs[idx], targetvals[idx][ioff],
		       delete [] targetvals );
	}
    }

    delete [] targetvals;

    return true;
}



bool testAnglesForDifferentSurveys();


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    RefMan<PreStack::VelocityBasedAngleComputer> computer =
				    new PreStack::VelocityBasedAngleComputer;

    PtrMan<IOObj> velobj = IOM().get( MultiID("100010.8") );
    mRunStandardTest( velobj.ptr(), "Input data is available" );

    computer->setMultiID( velobj->key() );
    const StepInterval<double> zrange(0,1.1,0.004), offsetrange(0,2500,500);
    FlatPosData fp;
    fp.setRange( true, offsetrange );
    fp.setRange( false, zrange );
    computer->setOutputSampling( fp );
    computer->setTrcKey( TrcKey(BinID(426,800)) );
    mRunStandardTest( computer->isOK(), "Angle computer is OK" );

    PtrMan<PreStack::Gather> angles = computer->computeAngles();
    mRunStandardTest( angles.ptr(), "Created angle data" );
    mRunStandardTest( isRawAngleOK(*angles), "Test raw angle values" );

    computer->setMovingAverageSmoother( 0.1f, HanningWindow::sName() );
    angles = computer->computeAngles();
    mRunStandardTest( angles && isMovingAverageAngleOK(*angles),
		      "Angle values after AVG filter" )

    computer->setFFTSmoother( 10.f, 15.f );
    angles = computer->computeAngles();
    mRunStandardTest( angles && isFFTAngleOK(*angles),
		      "Angle values after FFT Filter" )

    mRunStandardTest( testAnglesForDifferentSurveys(),
		      "Compared angle values in different surveys" );

    return true;
}


bool testAnglesForDifferentSurveys()
{
    BufferStringSet survnames;
    survnames.add( "F3_Test_Survey" )
	     .add( "F3_Test_Survey_XYinft" )
	     .add( "F3_Test_Survey_DepthM" )
	     .add( "F3_Test_Survey_DepthM_XYinft" )
	     .add( "F3_Test_Survey_DepthFT" )
	     .add( "F3_Test_Survey_DepthFT__XYinft_" );
    const int nrsurveys = survnames.size();

    const double zstart = 0.;
    TypeSet<StepInterval<double> > zrgs;
    zrgs.add( StepInterval<double> (zstart,1.1,0.004) );
    zrgs.add( StepInterval<double> (zstart,1200.,5.) );
    zrgs.add( StepInterval<double> (zstart,3930.,15.) );

    //TWT=933ms, Z~=965m, Z~=3165ft
    const int zids [] = { 233, 193, 211 };

    for ( int idx=0; idx<nrsurveys; idx++ )
    {
	const BufferString& survnm = survnames.get( idx );
	IOM().setSurvey( survnm );
	RefMan<PreStack::VelocityBasedAngleComputer> computer =
				new PreStack::VelocityBasedAngleComputer;

	PtrMan<IOObj> velobj = IOM().get( MultiID("100010.8") );
	if ( !velobj )
	{
	    od_cout() << survnm << " : Input data is not available.\n";
	    return false;
	}

	computer->setMultiID( velobj->key() );
	const StepInterval<double>& zrg = zrgs[idx/2];
	StepInterval<double> zrange(zrg), offsetrange(0,2500,500);
	FlatPosData fp;
	fp.setRange( true, offsetrange );
	fp.setRange( false, zrange );
	computer->setOutputSampling( fp );
	computer->setTrcKey( TrcKey(BinID(426,800)) );
	if ( !computer->isOK() )
	{
	    od_cout() << survnm << " : Angle computer is not OK.\n";
	    return false;
	}

	PtrMan<PreStack::Gather> angles = computer->computeAngles();
	if ( !angles )
	{
	    od_cout() << survnm ;
	    od_cout() << " : Computer did not succeed in making angle data\n";
	    return false;
	}
	else if ( !compareAngles(*angles,zids[idx/2],!SI().zIsTime(),
				 SI().zInFeet()) )
	{
	    od_cout() << survnm << " : Angle computer computed wrong values\n";
	    return false;
	}
    }

    return true;
}
