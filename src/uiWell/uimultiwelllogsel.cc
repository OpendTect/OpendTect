/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/


#include "uimultiwelllogsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellman.h"
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

    uiString dptlbl = UnitOfMeasure::zUnitAnnot( false, true, true );
    uiString timelbl = UnitOfMeasure::zUnitAnnot( true, true, true );
    uiStringSet units;
    units.add(uiString::emptyString()).add(dptlbl).add(timelbl);

    StringListInpSpec slis; const bool istime = SI().zIsTime();
    for ( int idx=0; idx<mMIN(zchoiceset.size(),units.size()); idx++ )
    {
	uiString msg = tr( "Start / stop %1" );
	msg.arg( units[idx] );
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
		Interval<float> zrg( SI().zRange(true) );
		zrg.scale( ztimefac_ );
		newgeninp->setValue( zrg );
	    }
	}
	zselectionflds_[idx]->attach( alignedBelow, zchoicefld_ );

    }

    uiString txt = tr("Distance above/below %1")
		.arg( UnitOfMeasure::zUnitAnnot(false,true,true) );

    abovefld_ = new uiGenInput( this, txt, FloatInpSpec(0).setName("above") );
    abovefld_->setElemSzPol( uiObject::Medium );
    abovefld_->valuechanged.notify( cb );
    abovefld_->attach( alignedBelow, zselectionflds_[0] );

    belowfld_ = new uiGenInput( this, uiString::emptyString(),
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


void uiWellZRangeSelector::setMarkers( const BufferStringSet& mrkrs )
{
    mDefWMS;
    wms->setMarkers( mrkrs );
}


void uiWellZRangeSelector::setMarkers( const Well::MarkerSet& mrkrs )
{
    mDefWMS;
    wms->setMarkers( mrkrs );
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
    , singlelog_(s.singlelog_)
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
	const bool zinft = SI().depthsInFeet();
	const float dptstep = zinft ? s.defmeterstep_*mToFeetFactorF
				    : s.defmeterstep_;
	const float timestep = SI().zStep()*ztimefac_;
	params().zstep_ = dptstep;
	uiString dptstpbuf = uiStrings::phrJoinStrings(uiStrings::sStep(),
					SI().getUiXYUnitString(true,true));
	uiString timelbl = UnitOfMeasure::zUnitAnnot( true, true, true );
	uiString timestpbuf = uiStrings::phrJoinStrings(uiStrings::sStep(),
								   timelbl);
	depthstepfld_ = new uiGenInput(this, dptstpbuf, FloatInpSpec(dptstep));
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
	    depthstepfld_->setValue( step );
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



uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const Setup& s,
				      const BufferStringSet* wellnms,
				      const BufferStringSet* lognms )
    : uiWellExtractParams(p,s)
    , singlewid_(0)
{
    init();
    update();
    if ( wellsfld_ && wellnms && wellnms->size() )
	wellsfld_->setChosen( *wellnms );

    if ( lognms && lognms->size() )
	logsfld_->setChosen( *lognms );
}


uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const Setup& s,
				      const DBKey& singlewid )
    : uiWellExtractParams(p,s)
    , singlewid_(&singlewid)
{
    init();
    update();
}


void uiMultiWellLogSel::init()
{
    const uiObject::SzPolicy hpol = uiObject::MedMax;
    const uiObject::SzPolicy vpol = uiObject::WideMax;
    const OD::ChoiceMode chmode =
	singlelog_ ? OD::ChooseOnlyOne : OD::ChooseAtLeastOne;
    uiListBox::Setup su( chmode, singlelog_ ? uiStrings::sLog() :
							    uiStrings::sLogs(),
			 singlewid_ ? uiListBox::LeftTop : uiListBox::AboveMid);
    logsfld_ = new uiListBox( this, su );
    logsfld_->setHSzPol( hpol );
    logsfld_->setVSzPol( vpol );

    wellsfld_ = 0; wellschoiceio_ = 0;
    zchoicefld_->attach( ensureBelow, logsfld_ );

    if ( !singlewid_ )
    {
	uiListBox::Setup suw( OD::ChooseAtLeastOne, uiStrings::sWells(),
			      uiListBox::AboveMid );
	wellsfld_ = new uiListBox( this, suw );
	wellsfld_->setHSzPol( hpol );
	wellsfld_->setVSzPol( vpol );
	mAttachCB( wellsfld_->selectionChanged,
		   uiMultiWellLogSel::updateLogsFldCB);
	mAttachCB( wellsfld_->itemChosen, uiMultiWellLogSel::updateLogsFldCB);
	logsfld_->attach( rightTo, wellsfld_ );

	wellschoiceio_ = new uiListBoxChoiceIO( *wellsfld_, "Well" );
	mAttachCB( wellschoiceio_->readDone,
		   uiMultiWellLogSel::readWellChoiceDone );
	mAttachCB( wellschoiceio_->storeRequested,
		   uiMultiWellLogSel::writeWellChoiceReq );
    }

    zchoicefld_->attach( alignedBelow, wellsfld_ ? wellsfld_ : logsfld_ );
}


void uiMultiWellLogSel::selectOnlyWritableWells()
{
    NotifyStopper ns( wellsfld_->selectionChanged );
    for ( int idx=0; idx<wellobjs_.size(); idx++ )
    {
	IOObj* ioobj = wellobjs_[idx];
	if ( !Well::Writer::isFunctional(*ioobj) )
	{
	    wellsfld_->removeItem( idx );
	    wellobjs_.removeSingle(idx);
	    delete ioobj;
	    idx--;
	}
    }

    updateLogsFldCB( 0 );
}


void uiMultiWellLogSel::onFinalise( CallBacker* )
{
    putToScreen();
}


uiMultiWellLogSel::~uiMultiWellLogSel()
{
    detachAllNotifiers();
    deepErase( wellobjs_ );
}


void uiMultiWellLogSel::update()
{
    clear();
    if ( wellsfld_ )
    {
	NotifyStopper ns( wellsfld_->selectionChanged );
	wellsfld_->setEmpty();
    }

    logsfld_->setEmpty();

    deepErase( wellobjs_ );

    Well::InfoCollector wic( false, false, false );
    if ( !wic.execute() ) return;

    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	const DBKey dbky = wic.ids()[iid];
	IOObj* ioobj = IOM().get( dbky );
	if ( !ioobj || ( singlewid_ && dbky != *singlewid_ ) )
	    continue;

	wellobjs_ += ioobj;

	if ( wellsfld_ )
	    wellsfld_->addItem( ioobj->uiName() );
    }

    updateLogsFldCB( 0 );
}


void uiMultiWellLogSel::updateLogsFldCB( CallBacker* )
{
    logsfld_->setEmpty();
    DBKeySet mids;
    getSelWellIDs( mids );
    if ( mids.isEmpty() )
	return;

    BufferStringSet availablelognms;
    BufferStringSet availablemrkrs;
    for ( int midx=0; midx<mids.size(); midx++ )
    {
	const DBKey wmid = mids[midx];
	RefMan<Well::Data> wd = new Well::Data;
	BufferStringSet lognms;
	if ( Well::MGR().isLoaded(wmid) )
	{
	    wd = Well::MGR().get( wmid );
	    if ( !wd ) continue;
	    wd->logs().getNames( lognms );
	}
	else
	{
	    Well::Reader* wrdr = new Well::Reader( wmid, *wd );
	    wrdr->getLogInfo( lognms );
	    if ( !wrdr->getMarkers() )
		continue;
	}

	BufferStringSet mrkrnms;
	wd->markers().getNames( mrkrnms );
	if ( midx == 0 )
	{
	    availablelognms = lognms;
	    availablemrkrs = mrkrnms;
	}
	else
	{
	    for ( int lidx=availablelognms.size()-1; lidx>=0; lidx-- )
	    {
		if (!lognms.isPresent(availablelognms.get(lidx)) )
		    availablelognms.removeSingle( lidx );
	    }

	    for ( int mrkidx=availablemrkrs.size()-1; mrkidx>=0; mrkidx-- )
	    {
		if (!mrkrnms.isPresent(availablemrkrs.get(mrkidx)) )
		    availablemrkrs.removeSingle( mrkidx );
	    }
	}
    }

    logsfld_->addItems( availablelognms );
    setMarkers( availablemrkrs );
}


void uiMultiWellLogSel::getSelWellIDs( DBKeySet& mids ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() )
    {
	mids.add( wellobjs_[0]->key() );
    }
    else if ( wellsfld_ )
    {
	for ( int idx=0; idx<wellsfld_->size(); idx++ )
	{
	    if ( wellsfld_->isChosen(idx) )
		mids.add( wellobjs_[idx]->key() );
	}
    }
}


void uiMultiWellLogSel::getSelWellIDs( BufferStringSet& wids ) const
{
    DBKeySet mids;
    getSelWellIDs( mids );
    for ( int idx=0; idx<mids.size(); idx++ )
	wids.add( mids[idx].toString() );
}


void uiMultiWellLogSel::setSelWellIDs( const BufferStringSet& idstrs )
{
    DBKeySet mids;
    for ( int idx=0; idx<idstrs.size(); idx++ )
	mids += DBKey::getFromString( idstrs.get(idx) );

    setSelWellIDs( mids );
}


void uiMultiWellLogSel::setSelWellIDs( const DBKeySet& ids )
{
    BufferStringSet wellnms;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( ids[idx] );
	if ( ioobj ) wellnms.add( ioobj->name() );
    }

    setSelWellNames( wellnms );
}


void uiMultiWellLogSel::getSelWellNames( BufferStringSet& wellnms ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() )
	wellnms.add( wellobjs_[0]->name() );
    else
	wellsfld_->getChosen( wellnms );
}


void uiMultiWellLogSel::setSelWellNames( const BufferStringSet& nms )
{ if ( wellsfld_ ) wellsfld_->setChosen( nms ); }

void uiMultiWellLogSel::getSelLogNames( BufferStringSet& nms ) const
{ logsfld_->getChosen( nms ); }

void uiMultiWellLogSel::setSelLogNames( const BufferStringSet& nms )
{ logsfld_->setChosen( nms ); }


void uiMultiWellLogSel::readWellChoiceDone( CallBacker* )
{
    if ( !wellschoiceio_ ) return;

    DBKeySet mids;
    for ( int idx=0; idx<wellschoiceio_->chosenKeys().size(); idx++ )
	mids += DBKey::getFromString( wellschoiceio_->chosenKeys().get(idx) );
    setSelWellIDs( mids );
}


void uiMultiWellLogSel::writeWellChoiceReq( CallBacker* )
{
    if ( !wellschoiceio_ ) return;

    wellschoiceio_->keys().setEmpty();
    for ( int idx=0; idx<wellobjs_.size(); idx++ )
	wellschoiceio_->keys().add( wellobjs_[idx]->key().toString() );
}
