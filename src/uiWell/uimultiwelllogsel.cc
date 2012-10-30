/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimultiwelllogsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "wellextractdata.h"
#include "wellmarker.h"

#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uiwellmarkersel.h"


#define mDefWMS mDynamicCastGet(uiWellMarkerSel*,wms,zselectionflds_[0])
#define mDefZFld(i) mDynamicCastGet(uiGenInput*,zfld,zselectionflds_[i])


uiWellZRangeSelector::uiWellZRangeSelector( uiParent* p, const Setup& s )
    : uiGroup( p, "Select Z Range" )
    , zchoicefld_(0)
    , abovefld_(0)
    , params_(new Well::ZRangeSelector()) 
    , ztimefac_(SI().showZ2UserFactor())
{
    const bool withzintime = s.withzintime_ && SI().zIsTime();
    const char** zchoice = Well::ExtractParams::ZSelectionNames();	
    BufferStringSet zchoiceset;

    for ( int idx=0; zchoice[idx]; idx++ )
    {
	zchoiceset.add( BufferString( zchoice[idx] ) );
	if ( !withzintime && idx ==1 )
	    break;
    }

    CallBack cb(mCB(this,uiWellZRangeSelector,getFromScreen));
    zchoicefld_ = new uiGenInput( this, s.txtofmainfld_,
					StringListInpSpec(zchoiceset) );
    zchoicefld_->valuechanged.notify( cb ); 
    setHAlignObj( zchoicefld_ );

    const bool zinft = SI().depthsInFeetByDefault();
    BufferString dptlbl = zinft ? "(ft)":"(m)";
    const char* units[] = { "",dptlbl.buf(),"(ms)",0 };

    StringListInpSpec slis; const bool istime = SI().zIsTime();
    for ( int idx=0; idx<zchoiceset.size(); idx++ )
    {
	BufferString msg( "Start / stop " );
	msg += units[idx];
	uiGenInput* newgeninp = 0; uiWellMarkerSel* newmarksel = 0;
	if ( !idx )
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

	    Well::ZRangeSelector::ZSelection zsel;
	    Well::ZRangeSelector::parseEnum( zchoiceset.get(idx), zsel );
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

    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";

    abovefld_ = new uiGenInput( this, txt, FloatInpSpec(0).setName("above") );
    abovefld_->setElemSzPol( uiObject::Medium );
    abovefld_->valuechanged.notify( cb );
    abovefld_->attach( alignedBelow, zselectionflds_[0] );

    belowfld_ = new uiGenInput( this, "", FloatInpSpec(0).setName("below") );
    belowfld_->setElemSzPol( uiObject::Medium );
    belowfld_->attach( rightOf, abovefld_ );
    belowfld_->valuechanged.notify( cb );

    setHAlignObj( zchoicefld_ );

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

    if ( selidx_ == 0 )
    {
	mDefWMS;
	wms->setInput( params_->topMarker(), true );
	wms->setInput( params_->botMarker(), false );
	abovefld_->setValue( params_->topOffset(), 0 ); 
	belowfld_->setValue( params_->botOffset(), 0 ); 
    }
    else
    {
	mDefZFld(selidx_);
	Interval<float> zrg( params_->getFixedRange() );
	if ( selidx_ > 1 )
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

    if ( selidx_ == 0 )
    {
	mDefWMS;
	params_->setTopMarker( wms->getText(true), abovefld_->getfValue(0,0) );
	params_->setBotMarker( wms->getText(false), belowfld_->getfValue(0,0) );
    }
    else
    {
	mDefZFld(selidx_);
	Interval<float> zrg( zfld->getFInterval() );
	if ( selidx_ > 1 )
	    zrg.scale( 1/ztimefac_ );
	params_->setFixedRange( zrg, true );
    }

    updateDisplayFlds();
}


void uiWellZRangeSelector::updateDisplayFlds()
{
    for ( int idx=0; idx<zselectionflds_.size(); idx++ )
	zselectionflds_[idx]->display( idx == selidx_ );

    abovefld_->display( selidx_ == 0 );
    belowfld_->display( selidx_ == 0 );
}


void uiWellZRangeSelector::setRange( Interval<float> zrg, bool istime )
{
    selidx_ = istime ? 2 : 1;
    mDefZFld(selidx_);
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
	zistimefld_ = new uiCheckBox( this, "Extract in time" );
	zistimefld_->attach( rightOf, zchoicefld_ );
	zistimefld_->activated.notify( cb );
    }


    if ( dostep_ )
    {
	const bool zinft = SI().depthsInFeetByDefault();
	const float dptstep = zinft ? s.defmeterstep_*mToFeetFactorF 
				    : s.defmeterstep_;
	const float timestep = SI().zStep()*ztimefac_;
	params().zstep_ = dptstep;
	BufferString stpbuf( "Step "); 
	BufferString dptstpbuf( stpbuf ); dptstpbuf += zinft ? "(ft)" : "(m )";
	BufferString timestpbuf( stpbuf ); timestpbuf += "(ms)";

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
	sampfld_ = new uiGenInput( this, "Log resampling method",
				StringListInpSpec(Stats::UpscaleTypeNames()) );
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
	float step = depthstepfld_->getfValue();
	if ( params().extractzintime_ ) 
	{
	    step = timestepfld_->getfValue();
	    step /= ztimefac_;
	}
	params().zstep_ = step;

	depthstepfld_->display( !params().extractzintime_ );
	timestepfld_->display( params().extractzintime_ );
    }

    if ( sampfld_ )
	params().samppol_ = (Stats::UpscaleType)(sampfld_->getIntValue());
}



uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const Setup& s ) 
    : uiWellExtractParams(p,s)
    , singlewid_(0)  
{
    init();
}

uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const Setup& s, 
					const MultiID& singlewid )
    : uiWellExtractParams(p,s)
    , singlewid_(&singlewid)  
{
    init();
}


void uiMultiWellLogSel::init()
{
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
	singlewid_ ? uiLabeledListBox::LeftTop : uiLabeledListBox::RightTop );
    logsfld_ = llbl->box();
    logsfld_->setMultiSelect( !singlelog_ );

    welllslblfld_ = 0;
    wellsfld_ = 0;
    zchoicefld_->attach( ensureBelow, llbl );

    if ( !singlewid_ )
    {
	uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
	wellsfld_ = llbw->box();
	llbl->attach( rightTo, llbw );
	welllslblfld_ = llbw;
    }
    zchoicefld_->attach( alignedBelow, singlewid_ ? llbl : welllslblfld_ );
    setStretch( 2, 0 );
}


void uiMultiWellLogSel::onFinalise( CallBacker* )
{
    update();
    putToScreen();
}


uiMultiWellLogSel::~uiMultiWellLogSel()
{
    deepErase( wellobjs_ );
}


void uiMultiWellLogSel::update()
{
    clear();

    if ( wellsfld_ )
	wellsfld_->setEmpty(); logsfld_->setEmpty();

    deepErase( wellobjs_ );

    Well::InfoCollector wic;
    uiTaskRunner tr( this );
    if ( !tr.execute(wic) ) return;

    BufferStringSet markernms;
    BufferStringSet lognms;
    Well::MarkerSet mrkrs;
    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	const MultiID& mid = *wic.ids()[iid];
	IOObj* ioobj = IOM().get( mid );
	if ( !ioobj || ( singlewid_ && mid != *singlewid_ ) ) 
	    continue;

	wellobjs_ += ioobj;

	const BufferStringSet& logs = *wic.logs()[iid];
	for ( int ilog=0; ilog<logs.size(); ilog++ )
	    lognms.addIfNew( logs.get(ilog) );

	mrkrs.append( *wic.markers()[iid] );

	if ( wellsfld_ )
	    wellsfld_->addItem( ioobj->name() );
    }

    if ( wellsfld_ )
	wellsfld_->selectAll( true );
    sort( mrkrs ); setMarkers( mrkrs );

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );
    const int prefnmidx = lognms.nearestMatch( prefpropnm_.buf() );
    if ( lognms.validIdx(prefnmidx) )
	logsfld_->setCurrentItem( prefnmidx );
}


void uiMultiWellLogSel::getSelWellIDs( BufferStringSet& wids ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() ) 
    {
	wids.add( wellobjs_[0]->key() );
    }
    else
    {
	for ( int idx=0; idx<wellsfld_->size(); idx++ )
	{
	    if ( wellsfld_->isSelected(idx) )
		wids.add( wellobjs_[idx]->key() );
	}
    }
} 


void uiMultiWellLogSel::getSelWellNames( BufferStringSet& wellnms ) const
{ 
    if ( singlewid_ && !wellobjs_.isEmpty() ) 
	wellnms.add( wellobjs_[0]->name() );
    else
	wellsfld_->getSelectedItems( wellnms ); 
}


void uiMultiWellLogSel::getSelLogNames( BufferStringSet& lognms ) const
{ logsfld_->getSelectedItems( lognms ); }

