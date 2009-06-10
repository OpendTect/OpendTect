/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.7 2009-06-10 08:07:46 cvsbruno Exp $";

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

    calcVelFactor( veluom_->symbol(), wtsetup->issonic_ );
    calcDensFactor( denuom_->symbol() );

    Well::MGR().release( wtsetup->wellid_ ); 
}


void WellTieUnitFactors::calcVelFactor( const char* velunit,
       						bool issonic )
{
    issonic ? calcSonicVelFactor( velunit ) : calcVelFactor( velunit );
}


void WellTieUnitFactors::calcSonicVelFactor( const char* velunit )
{
    double velfactor;
    if ( !strcmp( velunit, "None") )
	velfactor = 0.001*mFromFeetFactor;
    if ( !strcmp( velunit, "us/ft") )
	velfactor =  0.001*mFromFeetFactor;
    else if ( !strcmp(velunit, "ms/ft" ) )
	velfactor =  1*mFromFeetFactor;
    else if ( !strcmp( velunit, "s/ft" ) )
	velfactor =  1000*mFromFeetFactor;
    else if ( !strcmp( velunit, "us/m") )
	velfactor =  0.001;
    else if ( !strcmp( velunit, "ms/m") )
	velfactor =  1;
    else if ( !strcmp( velunit, "s/m") )
	velfactor =  1000;
    else
	velfactor = 0.001*mFromFeetFactor; // us/ft taken if no unit found

    velfactor_ = velfactor;
}


void WellTieUnitFactors::calcVelFactor( const char* velunit )
{
    double velfactor;
    if ( !strcmp( velunit, "None") )
	velfactor = 0.001 * mToFeetFactor;
    if ( !strcmp( velunit, "ft/us") )
	velfactor =  0.001 * mToFeetFactor;
    else if ( velunit, "ft/ms")
	velfactor =  1 * mToFeetFactor;
    else if ( !strcmp( velunit, "ft/s" ) )
	velfactor =  1000 * mToFeetFactor;
    else if ( !strcmp( velunit, "m/us") )
	velfactor =  1/0.001;
    else if ( !strcmp( velunit, "m/ms") )
	velfactor =  1;
    else if ( !strcmp( velunit, "m/s") )
	velfactor =  1/1000;
    else
	velfactor = 0.001 * mToFeetFactor; // ft/us taken if no unit found

    velfactor_ = velfactor;
}


void WellTieUnitFactors::calcDensFactor( const char* densunit )
{
    double denfactor;
    if ( !strcmp( densunit, "None") )
	denfactor = 1000;
    if ( !strcmp( densunit, "g/cc") )
	denfactor =  1000;
    else if ( !strcmp( densunit, "g/cm") )
	denfactor =  0.001;
    else if ( !strcmp( densunit, "kg/cc") )
	denfactor =  1000000;
    else if ( !strcmp( densunit, "kg/cm") )
	denfactor =  1;
    else
	denfactor = 1000; // g/cc taken if no unit found

    denfactor_ = denfactor;
}



WellTieParams::WellTieParams( const WellTieSetup& wts, Well::Data* wd,
			      const Attrib::DescSet& ads )
		: wtsetup_(wts)
		, wd_(*wd)
		, ads_(ads)	  
		, iscsavailable_(wd->checkShotModel())
		, timeintv_(0,0,0)
		, worksize_(0)
		, step_(20)	      
		, dispsize_(0)
		, nrdatacols_(10)
		, factors_(WellTieUnitFactors(&wts))
		, iscscorr_(iscsavailable_)
		, iscsdisp_(true)
		, currvellognm_(wts.vellognm_)
{
    corrstartdah_ = wd_.track().dah(0);
    corrstopdah_  = wd_.track().dah(wd_.track().size()-1);

    if ( wd_.checkShotModel() )
    {
	currvellognm_ = wtsetup_.corrvellognm_;
	WellTieCSCorr cscorr( wd_, *this );
    }

    attrnm_ = getAttrName(ads);
    resetDataParams(0);
    createColNames();
}


#define mStep 20
#define mComputeStepFactor SI().zStep()/mStep
bool WellTieParams::resetDataParams( CallBacker* )
{
    const float startdah = wd_.track().dah(0);
    const float stopdah  = wd_.track().dah(wd_.track().size()-1);

    setTimes( timeintv_, startdah, stopdah );
    setTimes( corrtimeintv_, corrstartdah_, corrstopdah_ );

    worksize_ = (int) ( (timeintv_.stop-timeintv_.start)/timeintv_.step );
    dispsize_ = (int) ( worksize_/mStep )-1;
    corrsize_ = (int) ( (corrtimeintv_.stop - corrtimeintv_.start )
	    		/(mStep*timeintv_.step) );

    if ( corrsize_>dispsize_ ) corrsize_ = dispsize_;

    return true;
}


bool WellTieParams::setTimes( StepInterval<float>& timeintv, 
			      float startdah, float stopdah )
{
    timeintv.start = wd_.d2TModel()->getTime( startdah );
    timeintv.stop  = wd_.d2TModel()->getTime( stopdah );
    timeintv.step  = mComputeStepFactor;

    if ( timeintv.step < 1e-6 )
	return false;

    if ( timeintv.start > timeintv_.stop )
	return false;
    return true;
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


void WellTieParams::createColNames()
{
    dptnm_ = "Depth";			colnms_.add( dptnm_ ); 
    timenm_ = "Time";  			colnms_.add( timenm_ );
    				        colnms_.add(wtsetup_.corrvellognm_);
    				 	colnms_.add( wtsetup_.vellognm_ );
    				 	colnms_.add( wtsetup_.denlognm_ );
    ainm_ = "Computed AI";	        colnms_.add( ainm_ );     
    refnm_ ="Computed Reflectivity";    colnms_.add( refnm_ );
    synthnm_ = "Synthetics";         	colnms_.add( synthnm_ );
    crosscorrnm_ = "Cross Correlation"; colnms_.add( crosscorrnm_ );
             				colnms_.add( attrnm_ );
}
