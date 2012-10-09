/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";

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


uiWellExtractParams::uiWellExtractParams( uiParent* p, const Setup& s )
    : uiGroup( p, "Select Z Range" )
    , stepfld_(0)  
    , sampfld_(0)  
    , zchoicefld_(0)
    , zistimefld_(0) 
    , prefpropnm_(s.prefpropnm_) 
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

    CallBack cb(mCB(this,uiWellExtractParams,getFromScreen));
    zchoicefld_ = new uiGenInput( this, s.txtofmainfld_,
					StringListInpSpec(zchoiceset) );
    zchoicefld_->valuechanged.notify( cb ); 
    setHAlignObj( zchoicefld_ );

    if ( SI().zIsTime() && s.withextractintime_ )
    {
	zistimefld_ = new uiCheckBox( this, "Extract in time" );
	zistimefld_->attach( rightOf, zchoicefld_ );
	zistimefld_->activated.notify( 
				mCB(this,uiWellExtractParams,extrInTimeCB) );
	zistimefld_->activated.notify( cb );
    }

    const bool zinft = SI().depthsInFeetByDefault();
    BufferString dptlbl = zinft ? "(ft)":"(m)";
    const char* units[] = { "",dptlbl.buf(),"(ms)",0 };
    const char* markernms[] = 	{ "Top Marker", "Bottom Marker", 0 };

    StringListInpSpec slis; 
    for ( int idx=0; idx<zchoiceset.size(); idx++ )
    {
	BufferString msg( "Start / stop " );
	msg += units[idx];
	zselectionflds_ += idx ? 
	      new uiGenInput(this,msg,FloatInpIntervalSpec())
	    : new uiGenInput( this,msg, slis.setName(markernms[0]),
				        slis.setName(markernms[1]) );

	zselectionflds_[idx]->setElemSzPol( uiObject::Medium );
	zselectionflds_[idx]->attach( alignedBelow, zchoicefld_ );
	zselectionflds_[idx]->attach( ensureBelow, zchoicefld_ );
	zselectionflds_[idx]->valuechanged.notify( cb );
    }

    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";
    abovefld_ = new uiGenInput( this, txt,
	    			FloatInpSpec(0).setName("Distance above") );
    abovefld_->setElemSzPol( uiObject::Medium );
    abovefld_->valuechanged.notify( cb );
    abovefld_->attach( alignedBelow, zselectionflds_[0] );
    belowfld_ = new uiGenInput( this, "", 
	    			FloatInpSpec(0).setName("Distance below") );
    belowfld_->setElemSzPol( uiObject::Medium );
    belowfld_->attach( rightOf, abovefld_ );
    belowfld_->valuechanged.notify( cb );

    if ( s.withzstep_ )
    {
	const float defstep =  zinft ? 5 : 1;
	stepfld_ = new uiGenInput( this, "Step (m ) ", FloatInpSpec(defstep) );
	stepfld_->setElemSzPol( uiObject::Small );
	if ( zistimefld_ )
	    stepfld_->attach( rightOf, zistimefld_ );
	else
	    stepfld_->attach( rightOf, zchoicefld_ ); 
	stepfld_->valuechanged.notify( cb );
    }

    if ( s.withsampling_ )
    {
	sampfld_ = new uiGenInput( this, "Log resampling method",
				StringListInpSpec(Stats::UpscaleTypeNames()) );
	sampfld_->valuechanged.notify( cb );
	sampfld_->attach( alignedBelow, abovefld_ );
    }
    attach_ = zchoicefld_;
}


void uiWellExtractParams::clear()
{
    markernms_.erase();
}


void uiWellExtractParams::addMarkers( const Well::MarkerSet& mrkrs )
{
    BufferStringSet markernms;
    for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms.addIfNew( mrkrs[imrk]->name() );

    addMarkers( markernms );
}


void uiWellExtractParams::addMarkers( const BufferStringSet& mrkrs )
{
    for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms_.addIfNew( mrkrs.get( imrk ) );

    BufferStringSet uimarkernms; 
    uimarkernms.addIfNew( Well::ExtractParams::sKeyDataStart() );
    uimarkernms.add( markernms_, false );
    uimarkernms.addIfNew( Well::ExtractParams::sKeyDataEnd() );
    StringListInpSpec slis( uimarkernms );
    zselectionflds_[0]->newSpec( slis, 0 );
    zselectionflds_[0]->newSpec( slis, 1 );
    zselectionflds_[0]->setText( uimarkernms.get(0).buf(), 0 );
    zselectionflds_[0]->setText(uimarkernms.get(uimarkernms.size()-1).buf(), 1);

    getFromScreen(0);
}


void uiWellExtractParams::putToScreen()
{
    if ( zistimefld_ )
	zistimefld_->setChecked( params_.extractzintime_ );
    selidx_ = (int)params_.zselection_ ;
    zchoicefld_->setValue( selidx_ );	

    const float ztimefac = SI().zDomain().userFactor();
    if ( selidx_ == 0 )
    {
	zselectionflds_[0]->setText(params_.topmrkr_,0);
	zselectionflds_[0]->setText(params_.botmrkr_,1);
	abovefld_->setValue( params_.above_, 0 ); 
	belowfld_->setValue( params_.below_, 0 ); 
    }
    else if ( selidx_ == 1 )
    {
	zselectionflds_[selidx_]->setValue( params_.zrg_ );
    }
    else
    {
	Interval<float> zrg( params_.getFixedRange() );
	zrg.scale( ztimefac );
	params_.setFixedRange( zrg, true );
    }


    if ( stepfld_ )
    {
	float step = params().zstep_; 
	if ( params().extractzintime_ ) 
	    step *= ztimefac;
	stepfld_->setValue( step );
    }

    if ( sampfld_ )
	sampfld_->setValue( (int)params_.samppol_ );

    updateDisplayFlds();
}


void uiWellExtractParams::extrInTimeCB( CallBacker* )
{
    if ( !stepfld_ )
	return;

    const bool zinft = SI().depthsInFeetByDefault();
    const bool intime = zistimefld_ ? zistimefld_->isChecked() : false;
    const float ztimefac = SI().zDomain().userFactor();
    const float defstep = intime ? ztimefac*SI().zStep() : zinft ? 5 : 1;
    NotifyStopper ns( stepfld_->valuechanged );
    BufferString steplbl( "Step" );
    steplbl += intime ? " (ms)" : zinft ? " (ft)" : " (m ) ";
    stepfld_->setTitleText( steplbl );
    stepfld_->setValue( defstep );
}



void uiWellExtractParams::getFromScreen( CallBacker* )
{
    selidx_ = zchoicefld_->getIntValue();

    params_.setEmpty();
    params_.zselection_ = Well::ExtractParams::ZSelection( 
	    				zchoicefld_->getIntValue() );

    params_.extractzintime_ = zistimefld_ ?  zistimefld_->isChecked() : false;
    const float ztimefac = SI().zDomain().userFactor();
    if ( selidx_ == 0 )
    {
	params_.topmrkr_ = zselectionflds_[0]->text(0);
	params_.botmrkr_ = zselectionflds_[0]->text(1);
	params_.above_ = abovefld_->getfValue(0,0); 
	params_.below_ = belowfld_->getfValue(0,0); 
    }
    else if ( selidx_ == 1 )
    {
	params_.setFixedRange( zselectionflds_[selidx_]->getFInterval(),false);
    }
    else
    {
	Interval<float> zrg( zselectionflds_[selidx_]->getFInterval() );
	zrg.scale( 1/ztimefac );
	params_.setFixedRange( zrg, true );
    }

    if ( stepfld_ )
    {
	float step = stepfld_->getfValue();
	if ( params().extractzintime_ ) 
	    step /= ztimefac;
	params_.zstep_ = step;
    }

    if ( sampfld_ )
	params_.samppol_ = (Stats::UpscaleType)(sampfld_->getIntValue());

    updateDisplayFlds();
}


void uiWellExtractParams::setRange( Interval<float> zrg, bool istime )
{
    selidx_ = istime ? 2 : 1;
    zchoicefld_->setValue( selidx_ );
    zselectionflds_[selidx_]->setValue( zrg );

    getFromScreen(0);
}


void uiWellZRangeSelector::updateDisplayFlds()
{
    for ( int idx=0; idx<zselectionflds_.size(); idx++ )
	zselectionflds_[idx]->display( idx == selidx_ );

    abovefld_->display( selidx_ == 0 );
    belowfld_->display( selidx_ == 0 );
}



uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const Setup& s ) 
    : uiWellExtractParams(p,s)
{
    uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
    wellsfld_ = llbw->box();
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
						   uiLabeledListBox::RightTop );
    singlelog_ = s.singlelog_;
    logsfld_ = llbl->box();
    logsfld_->setMultiSelect( !singlelog_ );
    llbl->attach( rightTo, llbw );
    welllslblfld_ = llbw;

    zchoicefld_->attach( ensureBelow, llbl );
    attach_->attach( alignedBelow, llbw );

    setStretch( 2, 0 );
    postFinalise().notify(mCB(this,uiMultiWellLogSel,onFinalise));
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

    wellsfld_->setEmpty(); logsfld_->setEmpty();
    deepErase( wellobjs_ );

    Well::InfoCollector wic;
    uiTaskRunner tr( this );
    if ( !tr.execute(wic) ) return;

    BufferStringSet markernms;
    BufferStringSet lognms;
    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	const MultiID& mid = *wic.ids()[iid];
	IOObj* ioobj = IOM().get( mid );
	if ( !ioobj ) continue;
	wellobjs_ += ioobj;
	wellids_ += mid;
	wellsfld_->addItem( ioobj->name() );

	const BufferStringSet& logs = *wic.logs()[iid];
	for ( int ilog=0; ilog<logs.size(); ilog++ )
	    lognms.addIfNew( logs.get(ilog) );

	const Well::MarkerSet& mrkrs = *wic.markers()[iid];
	addMarkers( mrkrs );
    }

    wellsfld_->selectAll( true );

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );

    const int prefnmidx = lognms.nearestMatch( prefpropnm_.buf() );
    if ( lognms.validIdx(prefnmidx) )
	logsfld_->setCurrentItem( prefnmidx );
}


void uiMultiWellLogSel::getSelWellIDs( BufferStringSet& wids ) const
{
    for ( int idx=0; idx<wellsfld_->size(); idx++ )
    {
	if ( wellsfld_->isSelected(idx) )
	    wids.add( wellobjs_[idx]->key() );
    }
} 


void uiMultiWellLogSel::getSelWellNames( BufferStringSet& wellnms ) const
{ wellsfld_->getSelectedItems( wellnms ); }


void uiMultiWellLogSel::getSelLogNames( BufferStringSet& lognms ) const
{ logsfld_->getSelectedItems( lognms ); }


void uiMultiWellLogSel::setSingleLogSel( bool yn )
{
    logsfld_->setMultiSelect( !yn );
}
