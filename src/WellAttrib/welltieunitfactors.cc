/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.29 2009-09-24 15:29:09 cvsbruno Exp $";

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


#define mComputeFactor (SI().zStep())
#define mMaxWorkSize (int)1.e5
#define mMinWorkSize (int)20
#define mTime(dah) d2t->getTime(dah)
bool Params::DataParams::resetTimeParams()
{
    const Well::D2TModel* d2t = wd_.d2TModel(); 
    if ( !d2t ) return false;
    const float stopdah  = wd_.track().dah(wd_.track().size()-1);

    if ( !corrdahs_.start ) corrdahs_.start = 0;
    if ( !corrdahs_.stop ) corrdahs_.stop = stopdah;

    timeintvs_.erase(); 
    timeintvs_ += StepInterval<float>( 0, mTime(stopdah), mComputeFactor/step_ );
    timeintvs_ += StepInterval<float>(timeintvs_[0]); timeintvs_[1].step*=step_;
    timeintvs_ += StepInterval<float>( mTime(corrdahs_.start), 
				       mTime(corrdahs_.stop), mComputeFactor);

    if ( timeintvs_[2].start<0 ) 
	timeintvs_[2].start = 0;
    if ( timeintvs_[2].stop > timeintvs_[0].stop )
	timeintvs_[2].stop = timeintvs_[0].stop;

    return true;
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
