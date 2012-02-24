/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uimultiwelllogsel.cc,v 1.5 2012-02-24 14:27:54 cvsbruno Exp $";

#include "uimultiwelllogsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "wellextractdata.h"
#include "wellmarker.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uitaskrunner.h"


uiWellZRangeSel::uiWellZRangeSel( uiParent* p,
      			bool withzstep,  bool withresampling )
    : uiGroup( p, "Select Z Range" )
    , stepfld_(0)  
    , logresamplfld_(0)  
{
    uiLabeledComboBox* llc0 = new uiLabeledComboBox( this, "Extract between" );
    topmarkfld_ = llc0->box();
    topmarkfld_->setName( "Top marker" );

    botmarkfld_ = new uiComboBox( this, "Bottom marker" );
    botmarkfld_->attach( rightOf, llc0 );
    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";
    abovefld_ = new uiGenInput( this, txt,
	    			FloatInpSpec(0).setName("Distance above") );
    abovefld_->attach( alignedBelow, llc0 );
    belowfld_ = new uiGenInput( this, "", 
	    			FloatInpSpec(0).setName("Distance below") );
    belowfld_->attach( rightOf, abovefld_ );

    if ( withzstep )
    {
	const bool zinft = SI().depthsInFeetByDefault();
	const float defstep = zinft ? 0.5 : 0.15;
	BufferString lbl = "Step "; lbl += zinft ? "(ft)" : "(m)";
	stepfld_ = new uiGenInput( this, lbl, FloatInpSpec(defstep) );
	stepfld_->attach( rightOf, botmarkfld_ );
    }

    if ( withresampling )
    {
	logresamplfld_ = new uiGenInput( this, "Log resampling method",
				StringListInpSpec(Stats::UpscaleTypeNames()) );
	logresamplfld_->attach( alignedBelow, abovefld_ );
    }
    attach_ = llc0;

    setStretch( 2, 2 );
}


void uiWellZRangeSel::clear()
{
    topmarkfld_->setEmpty(); botmarkfld_->setEmpty();
    markernms_.erase();
}


void uiWellZRangeSel::addMarkers( const Well::MarkerSet& mrkrs )
{
    BufferStringSet markernms;
    for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms.addIfNew( mrkrs[imrk]->name() );

    addMarkers( markernms );
}


void uiWellZRangeSel::addMarkers( const BufferStringSet& mrkrs )
{
    topmarkfld_->setEmpty(); botmarkfld_->setEmpty();

    for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms_.addIfNew( mrkrs.get( imrk ) );

    BufferStringSet uimarkernms; 
    uimarkernms.addIfNew( Well::TrackSampler::sKeyDataStart() );
    uimarkernms.add( markernms_, false );
    uimarkernms.addIfNew( Well::TrackSampler::sKeyDataEnd() );

    topmarkfld_->addItems( uimarkernms );
    botmarkfld_->addItems( uimarkernms );
    topmarkfld_->setCurrentItem( 0 );
    botmarkfld_->setCurrentItem( uimarkernms.size() - 1 );
}


void uiWellZRangeSel::getLimitMarkers( BufferString& top, 
				       BufferString& bot) const
{ top = topmarkfld_->text(); bot = botmarkfld_->text(); }


void uiWellZRangeSel::getLimitDists( float& top, float& bot ) const
{ top = abovefld_->getfValue(0,0); bot = belowfld_->getfValue(0,0); }


float uiWellZRangeSel::getStep() const
{ return stepfld_ ? stepfld_->getfValue() : mUdf(float); }


int uiWellZRangeSel::getResamplingType() const
{ return logresamplfld_ ? logresamplfld_->getIntValue() : -1; }


const char* uiWellZRangeSel::getTopMarker() const
{ return topmarkfld_->text(); }

const char* uiWellZRangeSel::getBottomMarker() const
{ return botmarkfld_->text();  }




uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, bool withresampling ) 
    : uiWellZRangeSel(p,withresampling)
{
    uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
    wellsfld_ = llbw->box();
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
						   uiLabeledListBox::RightTop );
    logsfld_ = llbl->box();
    llbl->attach( rightTo, llbw );

    botmarkfld_->attach( ensureBelow, llbl );
    attach_->attach( alignedBelow, llbw );
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

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );
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


