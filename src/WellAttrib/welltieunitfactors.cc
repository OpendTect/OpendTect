/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.36 2009-12-09 08:26:48 cvsbruno Exp $";

#include "welltieunitfactors.h"

#include "cubesampling.h"
#include "ioman.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllogset.h"
#include "wellman.h"

#include "welltiecshot.h"
#include "welltiesetup.h"

namespace WellTie
{

UnitFactors::UnitFactors( const WellTie::Setup* wtsetup )
	: velfactor_(0)
	, denfactor_(0)
	, veluom_(0)		    
	, denuom_(0)
{
    if ( !wtsetup ) return; 

    Well::Data* wd = Well::MGR().get( wtsetup->wellid_ ); 
    if ( !wd ) return; 

    const Well::Log* vellog =  wd->logs().getLog( wtsetup->vellognm_ );
    const Well::Log* denlog =  wd->logs().getLog( wtsetup->denlognm_ );
    if ( !vellog || !denlog ) 
	{ pErrMsg("Unable to access logs"); return; }

    const char* umlden = denlog->unitMeasLabel();
    denuom_ = UnitOfMeasure::getGuessed( umlden );
    const char* umlval = vellog->unitMeasLabel();
    veluom_ = UnitOfMeasure::getGuessed( umlval );

    if ( !denuom_ || !veluom_ )
	{ pErrMsg("No valid log units specified"); return; }

    velfactor_  = calcVelFactor( veluom_->symbol(), wtsetup->issonic_ );
    denfactor_ = calcDensFactor( denuom_->symbol() );
}


double UnitFactors::calcVelFactor( const char* velunit, bool issonic )
{
    return ( issonic ? calcSonicVelFactor( velunit ):calcVelFactor( velunit ) );
}


double UnitFactors::calcSonicVelFactor( const char* velunit )
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 0.001*mFromFeetFactor;
}


double UnitFactors::calcVelFactor( const char* velunit )
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 1000/mFromFeetFactor;
}


double UnitFactors::calcDensFactor( const char* densunit )
{
    const UnitOfMeasure* um = UoMR().get( densunit );
    return um ? um->userValue(1.0) : 1000;
}



Params::Params( const WellTie::Setup& wts, Well::Data* wd )
	: wtsetup_(wts)
	, uipms_(wd)				    
	, dpms_(wd,wts)				    
	, wd_(*wd)
{
    uipms_.iscscorr_ = ( wd_.haveCheckShotModel() && !wts.useexistingd2tm_ );

    dpms_.setUpCubeSampling(); 
    dpms_.currvellognm_ = uipms_.iscscorr_ ? wts.corrvellognm_ : wts.vellognm_;
    dpms_.createColNames();
    resetVelLogNm();

    for ( int idx=0; idx<3; idx++ )
	dpms_.timeintvs_ += StepInterval<float>( mUdf(float), mUdf(float), 0);

    resetParams();
}


void Params::resetVelLogNm()
{
    dpms_.currvellognm_ = uipms_.iscscorr_ ? wtsetup_.corrvellognm_
					   : wtsetup_.vellognm_;
    dpms_.dispcurrvellognm_ = uipms_.iscscorr_ ? dpms_.corrvellognm_
					       : dpms_.vellognm_;
}


void Params::DataParams::setUpCubeSampling()
{
    cs_ = new CubeSampling();
    SeisIOObjInfo oinf( IOM().get( wts_.seisid_ ) );
    if ( oinf.isOK() )
	oinf.getRanges( *cs_ );
}


#define mComputeFactor (SI().zStep())
#define mWorkFactor (mComputeFactor/20)
#define mMaxWorkSize (int)1.e5
#define mMinWorkSize (int)20
#define mMaxWorkArraySize (int)1.e5
#define mMinWorkArraySize (int)20
bool Params::DataParams::resetTimeParams()
{
    if ( !timeintvs_.size() ) return false;
    float tstart = 0; float tstop = d2T(wd_.track().dah(wd_.track().size()-1));
    if ( cs_ )
    {
	tstart = cs_->zrg.start;
	tstop = cs_->zrg.stop;
    }

    timeintvs_[0]=StepInterval<float>( tstart, tstop, mWorkFactor);
    timeintvs_[1]=StepInterval<float>(timeintvs_[0]); timeintvs_[1].step*=step_;
    timeintvs_[2].step = timeintvs_[1].step;

    if ( timeintvs_[0].nrSteps() > mMaxWorkArraySize ) return false;
    if ( timeintvs_[0].nrSteps() < mMinWorkArraySize ) return false;
    if ( timeintvs_[1].nrSteps() < 2 ) return false;
   
    if ( mIsUdf( timeintvs_[2].start ) || mIsUdf( timeintvs_[2].stop ) ) 
	timeintvs_[2] = timeintvs_[1];
    timeintvs_[2].limitTo( timeintvs_[1] );
    
    return true;
}


float Params::DataParams::d2T( float zval, bool istime ) const
{
    const Well::D2TModel* d2t = wd_.d2TModel(); 
    if ( !d2t ) return 0;
    
    return istime?  d2t->getTime( zval ) : d2t->getDepth( zval );
}


Interval<float> Params::DataParams::d2T( Interval<float> zintv, bool istime ) const
{
    Interval<float> zret( 0, 0);
    const Well::D2TModel* d2t = wd_.d2TModel(); 
    if ( !d2t ) return zret;

    if ( istime )
	zret.set( d2T( zintv.start ), d2T( zintv.stop ) );
    else
	zret.set( d2T( zintv.start, false ), d2T( zintv.stop, false ) );

    return zret;
} 
			  

void Params::DataParams::createColNames()
{
    colnms_.add( corrvellognm_ = wts_.corrvellognm_ );
    colnms_.add( vellognm_ = wts_.vellognm_ );
    colnms_.add( denlognm_ = wts_.denlognm_ );
    colnms_.add( ainm_ = "Computed AI" );     
    colnms_.add( refnm_ ="Computed Reflectivity" );
    colnms_.add( synthnm_ = "Synthetics" );
    colnms_.add( crosscorrnm_ = "Cross Correlation" );
    colnms_.add( seisnm_ = wts_.seisnm_ );
}

}; //namespace WellTie
