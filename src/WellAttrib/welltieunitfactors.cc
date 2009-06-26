/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.15 2009-06-26 09:39:56 cvsbruno Exp $";

#include "welltieunitfactors.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "math.h"
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


WellTieUnitFactors::WellTieUnitFactors( const WellTieSetup* wtsetup )
	    : velfactor_(0)
	    , denfactor_(0)
	    , veluom_(0)		    
	    , denuom_(0)
{
    if ( !wtsetup ) return; 

    Well::Data* wd = Well::MGR().get( wtsetup->wellid_ ); 
    if ( !wd ) return;; 

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

    Well::MGR().release( wtsetup->wellid_ ); 
}


double WellTieUnitFactors::calcVelFactor( const char* velunit, bool issonic )
{
    return ( issonic ? calcSonicVelFactor( velunit ):calcVelFactor( velunit ) );
}


double WellTieUnitFactors::calcSonicVelFactor( const char* velunit )
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 0.001*mFromFeetFactor;
}


double WellTieUnitFactors::calcVelFactor( const char* velunit )
{
    return ( 1 / calcSonicVelFactor( velunit ) );
}


double WellTieUnitFactors::calcDensFactor( const char* densunit )
{
    const UnitOfMeasure* um = UoMR().get( densunit );
    return um ? um->userValue(1.0) : 1000;
}



WellTieParams::WellTieParams( const WellTieSetup& wts, Well::Data* wd,
			      const Attrib::DescSet& ads )
		: wtsetup_(wts)
		, uipms_(wd)				    
		, dpms_(wd,wts)				    
		, wd_(*wd)
		, ads_(ads)	  
		, factors_(WellTieUnitFactors(&wts))
{
    dpms_.corrstartdah_ = wd_.track().dah(0);
    dpms_.corrstopdah_  = wd_.track().dah(wd_.track().size()-1);
    dpms_.currvellognm_ = wts.vellognm_;

    if ( wd_.checkShotModel() )
    {
	dpms_.currvellognm_ = wtsetup_.corrvellognm_;
	WellTieCSCorr cscorr( wd_, *this );
    }

    dpms_.attrnm_ = getAttrName(ads);
    dpms_.createColNames();
    resetParams();
}


BufferString WellTieParams::getAttrName( const Attrib::DescSet& ads ) const
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

bool WellTieParams::resetParams()
{
    if ( !dpms_.resetDataParams() )
	return false;
    //TODO this should be an easiest way to set the vellognm than using 
    // one name for display and one for data!
    resetVellLognm();
    return true;
}


void WellTieParams::resetVellLognm()
{
    dpms_.currvellognm_ = uipms_.iscscorr_? wtsetup_.corrvellognm_
					  : wtsetup_.vellognm_;
    dpms_.dispcurrvellognm_ = uipms_.iscscorr_? dpms_.corrvellognm_
					      : dpms_.vellognm_;
}


#define mStep 20
#define mComputeStepFactor SI().zStep()/mStep
bool WellTieParams::DataParams::resetDataParams()
{
    const float startdah = wd_.track().dah(0);
    const float stopdah  = wd_.track().dah(wd_.track().size()-1);

    setTimes( timeintv_, startdah, stopdah );
    setTimes( corrtimeintv_, corrstartdah_, corrstopdah_ );
    setDepths( timeintv_, dptintv_ );

    //TODO: change structure to get time and corrtime ALWAYS start at 0.
    //->no use to update startintv anymore!
    timeintv_.start = 0;
    corrtimeintv_.start = 0;

    worksize_ = (int) ( (timeintv_.stop-timeintv_.start)/timeintv_.step );
    dispsize_ = (int) ( worksize_/mStep )-1;
    corrsize_ = (int) ( (corrtimeintv_.stop-corrtimeintv_.start )
	    		 	/(mStep*timeintv_.step) );

    if ( corrsize_>dispsize_ ) corrsize_ = dispsize_;
    
    return true;
}


bool WellTieParams::DataParams::setTimes( StepInterval<double>& timeintv, 
			      float startdah, float stopdah )
{
    timeintv.start = wd_.d2TModel()->getTime( startdah );
    timeintv.stop  = wd_.d2TModel()->getTime( stopdah );
    timeintv.step  = mComputeStepFactor;

    if ( timeintv.step < 1e-6 )
	return false;

    if ( timeintv.start > timeintv.stop )
	return false;
    return true;
}


bool WellTieParams::DataParams::setDepths( const StepInterval<double>& timeintv,					   StepInterval<double>& dptintv )
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return false;

    dptintv.start = d2tm->getDepth( timeintv.start );
    dptintv.stop  = d2tm->getDepth( timeintv.stop );
    return true;
}


void WellTieParams::DataParams::createColNames()
{
    dptnm_ = "Depth";			      colnms_.add( dptnm_ ); 
    timenm_ = "Time";  			      colnms_.add( timenm_ );
    corrvellognm_ = wts_.corrvellognm_;       colnms_.add( corrvellognm_ );
    vellognm_ = wts_.vellognm_; 	      colnms_.add( wts_.vellognm_ );
    denlognm_ = wts_.denlognm_;		      colnms_.add( wts_.denlognm_ );
    ainm_ = "Computed AI";	              colnms_.add( ainm_ );     
    refnm_ ="Computed Reflectivity";          colnms_.add( refnm_ );
    synthnm_ = "Synthetics";         	      colnms_.add( synthnm_ );
    crosscorrnm_ = "Cross Correlation";       colnms_.add( crosscorrnm_ );
             				      colnms_.add( attrnm_ );

    BufferString add2name = "'"; 
    vellognm_ += add2name;
    denlognm_ += add2name;		
    corrvellognm_ += add2name;
}


WellTieParams::uiParams::uiParams( const Well::Data* d)
    	: wd_(*d)
	, iscsavailable_(d->checkShotModel())
	, iscscorr_(d->checkShotModel())
	, iscsdisp_(false)
	, ismarkerdisp_(d->haveMarkers())
	, iszinft_(false)
{}




