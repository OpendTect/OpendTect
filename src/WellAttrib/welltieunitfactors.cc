/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.28 2009-09-23 11:50:08 cvsbruno Exp $";

#include "welltieunitfactors.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "unitofmeasure.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
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



Params::Params( const WellTie::Setup& wts, Well::Data* wd,
		const Attrib::DescSet& ads )
	: wtsetup_(wts)
	, uipms_(wd)				    
	, dpms_(wd,wts)				    
	, wd_(*wd)
	, ads_(ads)	  
{
    dpms_.corrstartdah_ = wd_.track().dah(0);
    dpms_.corrstopdah_  = wd_.track().dah(wd_.track().size()-1);
    dpms_.currvellognm_ = wts.vellognm_;
    dpms_.attrnm_ = getAttrName( ads );
    if ( wd_.checkShotModel() )
	dpms_.currvellognm_ = wtsetup_.corrvellognm_;
    dpms_.createColNames();
    resetVelLogNm();

    resetParams();
}


BufferString Params::getAttrName( const Attrib::DescSet& ads ) const
{
    const Attrib::Desc* ad = ads.getDesc( wtsetup_.attrid_ );
    if ( !ad ) return 0;

    Attrib::SelInfo attrinf( &ads, 0, ads.is2D() );
    BufferStringSet bss;
    SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(0) ) );
    sii.getDefKeys( bss, true );
    const char* defkey = bss.get(0).buf();
    BufferString attrnm = ad->userRef();
    return SeisIOObjInfo::defKey2DispName(defkey,attrnm);
}


void Params::resetVelLogNm()
{
    dpms_.currvellognm_ = uipms_.iscscorr_ ? wtsetup_.corrvellognm_
					   : wtsetup_.vellognm_;
    dpms_.dispcurrvellognm_ = uipms_.iscscorr_ ? dpms_.corrvellognm_
					       : dpms_.vellognm_;
}


#define mMaxWorkSize (int)1.e5
#define mComputeStepFactor (SI().zStep()/step_)
#define mMinWorkSize (int)20
bool Params::DataParams::resetDataParams()
{
    const float startdah = wd_.track().dah(0);
    const float stopdah  = wd_.track().dah(wd_.track().size()-1);

    setTimes( timeintv_, startdah, stopdah );
    setTimes( corrtimeintv_, corrstartdah_, corrstopdah_ );
    timeintv_.start = 0;
    setDepths( timeintv_, dptintv_ );

    if ( corrtimeintv_.start<0 ) corrtimeintv_.start = 0; 
    if ( corrtimeintv_.stop>timeintv_.stop ) corrtimeintv_.stop=timeintv_.stop;

    worksize_ = getArraySize( timeintv_ );
    corrsize_ = getArraySize( corrtimeintv_ );
    dispsize_ = (int) ( worksize_/step_ );

    if ( corrsize_>dispsize_ ) corrsize_ = dispsize_;
    if ( worksize_ > mMaxWorkSize || worksize_ < mMinWorkSize ) return false;
    if ( dispsize_ < 2 || timeintv_.step < 1e-6 ) return false;
    
    return true;
}

#define mGetD2T(act) const Well::D2TModel* d2t = wd_.d2TModel(); if (!d2t) act;
bool Params::DataParams::setTimes( StepInterval<double>& timeintv, 
			      float startdah, float stopdah )
{
    mGetD2T( return false )
    timeintv.start = d2t->getTime( startdah );
    timeintv.stop  = d2t->getTime( stopdah );
    timeintv.step  = mComputeStepFactor;
    timeintv.sort();

    return true;
}


bool Params::DataParams::setDepths( const StepInterval<double>& timeintv,					   StepInterval<double>& dptintv )
{
    mGetD2T( return false )
    dptintv.start = d2t->getDepth( timeintv.start );
    dptintv.stop  = d2t->getDepth( timeintv.stop );
    return true;
}


int Params::DataParams::getArraySize( StepInterval<double>& timeintv ) const 
{
    return (int) ( (timeintv_.stop-timeintv_.start)/timeintv_.step );
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
    colnms_.add( attrnm_ );
}

}; //namespace WellTie
