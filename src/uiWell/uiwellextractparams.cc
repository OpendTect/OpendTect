/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Prajjaval Singh
Date:          Oct 2016
________________________________________________________________________

-*/


#include "uiwellextractparams.h"

#include "ioobj.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"

#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiwellmarkersel.h"


#define	cMarkersFld	0
#define	cDepthFld	1
#define	cTWTFld		2
#define mDefWMS mDynamicCastGet(uiWellMarkerSel*,wms, \
				zselectionflds_[cMarkersFld])
#define mGetZFld(i) \
    int zfldidx = i; \
    if ( zfldidx >= zselectionflds_.size() ) \
	zfldidx = zselectionflds_.size()-1; \
    mDynamicCastGet(uiGenInput*,zfld,zselectionflds_[zfldidx])


uiWellZRangeSelector::uiWellZRangeSelector( uiParent* p, const Setup& s )
    : uiGroup( p, "Select Z Range" )
    , zchoicefld_(0)
    , abovefld_(0)
    , params_(new Well::ZRangeSelector())
    , ztimefac_(SI().showZ2UserFactor())
{
    const bool withzintime = s.withzintime_ && SI().zIsTime();
    uiStringSet zchoiceset;

    for ( int idx=0; idx<Well::ExtractParams::ZSelectionDef().size(); idx++ )
    {
	zchoiceset.add( Well::ExtractParams::ZSelectionDef().strings()[idx] );
	if ( !withzintime && idx ==1 )
	    break;
    }

    CallBack cb( mCB(this,uiWellZRangeSelector,getFromScreen) );
    zchoicefld_ = new uiGenInput( this, toUiString(s.txtofmainfld_),
					StringListInpSpec(zchoiceset) );
    zchoicefld_->valuechanged.notify( cb );
    setHAlignObj( zchoicefld_ );

    uiString dptlbl = UnitOfMeasure::zUnitAnnot( false, true );
    uiString timelbl = UnitOfMeasure::zUnitAnnot( true, true );
    uiStringSet units;
    units.add(uiString::empty()).add(dptlbl).add(timelbl);

    StringListInpSpec slis; const bool istime = SI().zIsTime();
    for ( int idx=0; idx<mMIN(zchoiceset.size(),units.size()); idx++ )
    {
	uiString msg = tr("Start / stop").withUnit( units[idx] );
	uiGenInput* newgeninp = 0; uiWellMarkerSel* newmarksel = 0;
	if ( idx == 0 )
	{
	    newmarksel = new uiWellMarkerSel( this,
				uiWellMarkerSel::Setup(false) );
	    newmarksel->mrkSelDone.notify( cb );
	    zselectionflds_ += newmarksel;
	}
	else
	{
	    newgeninp = new uiGenInput(this,msg,FloatInpIntervalSpec());
	    zselectionflds_ += newgeninp;
	    newgeninp->setElemSzPol( uiObject::Medium );
	    newgeninp->valuechanged.notify( cb );

            const char* key =
                    Well::ExtractParams::ZSelectionDef().getKeyForIndex(idx);
	    const Well::ZRangeSelector::ZSelection zsel =
		Well::ZRangeSelector::ZSelectionDef().parse( key );
	    if ( (zsel == Well::ZRangeSelector::Times && istime) ||
		 (zsel == Well::ZRangeSelector::Depths && !istime) )
	    {
		Interval<float> zrg( SI().zRange(OD::UsrWork) );
		zrg.scale( ztimefac_ );
		newgeninp->setValue( zrg );
	    }
	}
	zselectionflds_[idx]->attach( alignedBelow, zchoicefld_ );

    }

    uiString txt = tr("Distance above/below")
		.withUnit( UnitOfMeasure::zUnitAnnot(false,true) );

    abovefld_ = new uiGenInput( this, txt, FloatInpSpec(0).setName("above") );
    abovefld_->setElemSzPol( uiObject::Medium );
    abovefld_->valuechanged.notify( cb );
    abovefld_->attach( alignedBelow, zselectionflds_[0] );

    belowfld_ = new uiGenInput( this, uiString::empty(),
                                FloatInpSpec(0).setName("below") );
    belowfld_->setElemSzPol( uiObject::Medium );
    belowfld_->attach( rightOf, abovefld_ );
    belowfld_->valuechanged.notify( cb );

    postFinalise().notify(mCB(this,uiWellZRangeSelector,onFinalise));
}


uiWellZRangeSelector::~uiWellZRangeSelector()
{
    delete params_;
}


void uiWellZRangeSelector::onFinalise( CallBacker* )
{
    putToScreen();
}


void uiWellZRangeSelector::clear()
{
}


void uiWellZRangeSelector::setRangeSel( const Well::ZRangeSelector& sel )
{
    delete params_;
    params_ = sel.clone();
    putToScreen();
}


void uiWellZRangeSelector::setMarkers( const BufferStringSet& mrkrs,
				       const TypeSet<Color>& colors )
{
    mDefWMS;
    wms->setMarkers( mrkrs, colors );
}


void uiWellZRangeSelector::setMarkers( const Well::MarkerSet& mrkrs )
{
    mDefWMS;
    wms->setMarkers( mrkrs );
}


void uiWellZRangeSelector::fillWithAllMarkers()
{
    mDefWMS;
    wms->fillWithAll();
}


void uiWellZRangeSelector::putToScreen()
{
    selidx_ = (int)params_->zselection_ ;
    zchoicefld_->setValue( selidx_ );

    if ( selidx_ == cMarkersFld )
    {
	mDefWMS;
	wms->setInput( params_->topMarker(), true );
	wms->setInput( params_->botMarker(), false );
	abovefld_->setValue( params_->topOffset(), 0 );
	belowfld_->setValue( params_->botOffset(), 0 );
    }
    else
    {
	mGetZFld( selidx_ );
	Interval<float> zrg( params_->getFixedRange() );
	if ( selidx_ == cTWTFld )
	    zrg.scale( ztimefac_ );

	zfld->setValue( zrg );
    }

    updateDisplayFlds();
}


void uiWellZRangeSelector::getFromScreen( CallBacker* )
{
    selidx_ = zchoicefld_->getIntValue();

    params_->setEmpty();
    params_->zselection_ = Well::ExtractParams::ZSelection(
					zchoicefld_->getIntValue() );

    if ( selidx_ == cMarkersFld )
    {
	mDefWMS;
	params_->setTopMarker( wms->getText(true), abovefld_->getFValue(0,0) );
	params_->setBotMarker( wms->getText(false), belowfld_->getFValue(0,0) );
    }
    else
    {
	mGetZFld( selidx_);
	Interval<float> zrg( zfld->getFInterval() );
	const bool isintime = selidx_ == cTWTFld;
	if ( isintime )
	    zrg.scale( 1/ztimefac_ );

	params_->setFixedRange( zrg, isintime );
    }

    updateDisplayFlds();
}


void uiWellZRangeSelector::updateDisplayFlds()
{
    for ( int idx=0; idx<zselectionflds_.size(); idx++ )
	zselectionflds_[idx]->display( idx == selidx_ );

    abovefld_->display( selidx_ == cMarkersFld );
    belowfld_->display( selidx_ == cMarkersFld );
}


void uiWellZRangeSelector::setRange( Interval<float> zrg, bool istime )
{
    selidx_ = istime ? cDepthFld : cTWTFld;
    mGetZFld(selidx_);
    zchoicefld_->setValue( selidx_ );
    zfld->setValue( zrg );

    getFromScreen(0);
}




uiWellExtractParams::uiWellExtractParams( uiParent* p, const Setup& s )
    : uiWellZRangeSelector( p, s )
    , depthstepfld_(0)
    , timestepfld_(0)
    , sampfld_(0)
    , zistimefld_(0)
    , dostep_(s.withzstep_)
    , prefpropnm_(s.prefpropnm_)
{
    delete params_;
    params_ = new Well::ExtractParams();

    CallBack cb(mCB(this,uiWellExtractParams,getFromScreen));

    if ( SI().zIsTime() && s.withextractintime_ )
    {
	zistimefld_ = new uiCheckBox( this, tr("Extract in time") );
	zistimefld_->attach( rightOf, zchoicefld_ );
	zistimefld_->activated.notify( cb );
    }

    if ( dostep_ )
    {
	const float dptstep = s.defmeterstep_;
	const float timestep = SI().zStep()*ztimefac_;
	params().zstep_ = dptstep;
	uiString dptstpbuf = uiStrings::sStep().withSurvXYUnit();
	uiString timelbl = UnitOfMeasure::zUnitAnnot( true, true );
	uiString timestpbuf = tr("%1 %2").arg( uiStrings::sStep() )
					 .arg( timelbl );

	const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
	const float zstepval = uom ? uom->userValue(dptstep) : dptstep;
	depthstepfld_ = new uiGenInput(this, dptstpbuf, FloatInpSpec(zstepval));
	timestepfld_ = new uiGenInput(this, timestpbuf, FloatInpSpec(timestep));
	depthstepfld_->setElemSzPol( uiObject::Small );
	timestepfld_->setElemSzPol( uiObject::Small );

	if ( zistimefld_ )
	{
	    depthstepfld_->attach( rightOf, zistimefld_ );
	    timestepfld_->attach( rightOf, zistimefld_ );
	}
	else
	{
	    depthstepfld_->attach( rightOf, zchoicefld_ );
	    timestepfld_->attach( rightOf, zchoicefld_ );
	}

	depthstepfld_->valuechanged.notify( cb );
	timestepfld_->valuechanged.notify( cb );
    }

    if ( s.withsampling_ )
    {
	sampfld_ = new uiGenInput( this, tr("Log resampling method"),
				StringListInpSpec(Stats::UpscaleTypeDef()) );
	sampfld_->setValue( Stats::UseAvg );
	sampfld_->valuechanged.notify( cb );
	sampfld_->attach( alignedBelow, abovefld_ );
    }
}


void uiWellExtractParams::onFinalise( CallBacker* )
{
    putToScreen();
}


void uiWellExtractParams::putToScreen()
{
    if ( zistimefld_ )
	zistimefld_->setChecked( params().extractzintime_ );

    uiWellZRangeSelector::putToScreen();

    if ( dostep_ )
    {
	float step = params().zstep_;
	if ( params().extractzintime_ )
	{
	    step *= ztimefac_;
	    timestepfld_->setValue( step );
	}
	else
	{
	    const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
	    step = uom ? uom->userValue(step) : step;
	    depthstepfld_->setValue( step );
	}
    }

    if ( sampfld_ )
	sampfld_->setValue( (int)params().samppol_ );
}


void uiWellExtractParams::updateDisplayFlds()
{
    uiWellZRangeSelector::updateDisplayFlds();
    if ( dostep_ )
    {
	depthstepfld_->display( !params().extractzintime_ );
	timestepfld_->display( params().extractzintime_ );
    }
}


void uiWellExtractParams::getFromScreen( CallBacker* cb )
{
    uiWellZRangeSelector::getFromScreen( cb );

    if ( zistimefld_ )
	params().extractzintime_ = zistimefld_->isChecked();

    if ( dostep_ )
    {
	float step = depthstepfld_->getFValue();
	const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
	step = uom ? uom->getSIValue(step) : step;
	if ( params().extractzintime_ )
	{
	    step = timestepfld_->getFValue();
	    step /= ztimefac_;
	}
	params().zstep_ = step;

	depthstepfld_->display( !params().extractzintime_ );
	timestepfld_->display( params().extractzintime_ );
    }

    if ( sampfld_ )
	params().samppol_ = (Stats::UpscaleType)(sampfld_->getIntValue());
}
