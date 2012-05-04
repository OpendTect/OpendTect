/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uimultiwelllogsel.cc,v 1.19 2012-05-04 13:31:55 cvsbruno Exp $";

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

uiWellZRangeSelector::uiWellZRangeSelector( uiParent* p, const Setup& s )
    : uiGroup( p, "Select Z Range" )
    , zchoicefld_(0)
    , abovefld_(0)
    , params_(new Well::ZRangeSelector()) 
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
	zselectionflds_[idx]->valuechanged.notify( cb );
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

    uiWellZRangeSelector::getFromScreen(0);
}


uiWellZRangeSelector::~uiWellZRangeSelector()
{
    delete params_;
}


void uiWellZRangeSelector::clear()
{
    markernms_.erase();
}


void uiWellZRangeSelector::addMarkers( const Well::MarkerSet& mrkrs )
{
    BufferStringSet markernms;
    for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms.addIfNew( mrkrs[imrk]->name() );

    addMarkers( markernms );
}


void uiWellZRangeSelector::addMarkers( const BufferStringSet& mrkrs )
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

    uiWellZRangeSelector::getFromScreen(0);
}


void uiWellZRangeSelector::putToScreen()
{
    selidx_ = (int)params_->zselection_ ;
    zchoicefld_->setValue( selidx_ );	

    if ( selidx_ == 0 )
    {
	zselectionflds_[0]->setText(params_->topMarker(),0);
	zselectionflds_[0]->setText(params_->botMarker(),1);
	abovefld_->setValue( params_->topOffset(), 0 ); 
	belowfld_->setValue( params_->botOffset(), 0 ); 
    }
    else
	zselectionflds_[selidx_]->setValue( params_->getFixedRange() );

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
	params_->setTopMarker( zselectionflds_[0]->text(0), 
			       abovefld_->getfValue(0,0) );
	params_->setBotMarker( zselectionflds_[0]->text(1), 
			       belowfld_->getfValue(0,0) );
    }
    else
	params_->setFixedRange( zselectionflds_[selidx_]->getFInterval(), 
				selidx_ == 2 );

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
    zchoicefld_->setValue( selidx_ );	
    zselectionflds_[selidx_]->setValue( zrg );

    getFromScreen(0);
}




uiWellExtractParams::uiWellExtractParams( uiParent* p, const Setup& s )
    : uiWellZRangeSelector( p, s )
    , stepfld_(0)  
    , sampfld_(0)  
    , zistimefld_(0) 
{
    delete params_;
    params_ = new Well::ExtractParams();

    CallBack cb(mCB(this,uiWellExtractParams,getFromScreen));

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

    if ( s.withzstep_ )
    {
	const float defstep =  zinft ? 0.5 : 0.15;
	stepfld_ = new uiGenInput( this, "Step (ms) ", FloatInpSpec(defstep) );
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
    postFinalise().notify( mCB(this,uiWellExtractParams,extrInTimeCB) );
}


void uiWellExtractParams::putToScreen()
{
    NotifyStopper ns( zistimefld_->activated );
    if ( zistimefld_ )
	zistimefld_->setChecked( params().extractzintime_ );

    uiWellZRangeSelector::putToScreen();

    NotifyStopper ns1( stepfld_->valuechanged );
    if ( stepfld_ )
	stepfld_->setValue( params().zstep_ );

    NotifyStopper ns2( sampfld_->valuechanged );
    if ( sampfld_ )
	sampfld_->setValue( (int)params().samppol_ );
}


void uiWellExtractParams::extrInTimeCB( CallBacker* )
{
    if ( !zistimefld_ || !stepfld_ )
	return;

    const bool zinft = SI().depthsInFeetByDefault();
    const bool intime = zistimefld_->isChecked();
    const float defstep = intime ? SI().zStep() : zinft ? 0.5 : 0.15;
    NotifyStopper ns( stepfld_->valuechanged );
    BufferString steplbl( "Step" );
    steplbl += intime ? " (ms)" : zinft ? " (ft)" : " (meters) ";
    stepfld_->setTitleText( steplbl );
    stepfld_->setValue( defstep );
}


void uiWellExtractParams::getFromScreen( CallBacker* cb )
{
    uiWellZRangeSelector::getFromScreen( cb );

    if ( zistimefld_ ) 
	params().extractzintime_ = zistimefld_->isChecked();

    if ( stepfld_ )
	params().zstep_ = stepfld_->getfValue();

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


    postFinalise().notify(mCB(this,uiMultiWellLogSel,onFinalise));
}


void uiMultiWellLogSel::onFinalise( CallBacker* )
{
    for ( int idx=0; idx<zselectionflds_.size(); idx++ )
    {
	zselectionflds_[idx]->display( true );
    }
    belowfld_->display( true  );
    abovefld_->display( true );

    update();
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

	const Well::MarkerSet& mrkrs = *wic.markers()[iid];
	addMarkers( mrkrs );

	if ( wellsfld_ )
	    wellsfld_->addItem( ioobj->name() );
    }

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );
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

