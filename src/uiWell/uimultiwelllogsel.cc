/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uimultiwelllogsel.cc,v 1.1 2011-01-21 14:44:49 cvsbruno Exp $";

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


uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p )
	: uiGroup(p)
{
    uiLabeledListBox* llbw = new uiLabeledListBox( this, "Wells", true );
    wellsfld_ = llbw->box();
    uiLabeledListBox* llbl = new uiLabeledListBox( this, "Logs", true,
						   uiLabeledListBox::RightTop );
    logsfld_ = llbl->box();
    llbl->attach( rightTo, llbw );

    uiLabeledComboBox* llc0 = new uiLabeledComboBox( this, "Extract between" );
    topmarkfld_ = llc0->box();
    topmarkfld_->setName( "Top marker" );
    llc0->attach( alignedBelow, llbw );

    botmarkfld_ = new uiComboBox( this, "Bottom marker" );
    botmarkfld_->attach( rightOf, llc0 );
    botmarkfld_->attach( ensureBelow, llbl );
    BufferString txt = "Distance above/below ";
    txt += SI().depthsInFeetByDefault() ? "(ft)" : "(m)";
    abovefld_ = new uiGenInput( this, txt,
	    			FloatInpSpec(0).setName("Distance above") );
    abovefld_->attach( alignedBelow, llc0 );
    belowfld_ = new uiGenInput( this, "", 
	    			FloatInpSpec(0).setName("Distance below") );
    belowfld_->attach( rightOf, abovefld_ );

    init();
}


void uiMultiWellLogSel::init()
{
    wellsfld_->setEmpty(); logsfld_->setEmpty();
    topmarkfld_->setEmpty(); botmarkfld_->setEmpty();
    deepErase( wellobjs_ );

    Well::InfoCollector wic;
    uiTaskRunner tr( this );
    if ( !tr.execute(wic) ) return;

    BufferStringSet markernms, lognms;
    markernms.add( Well::TrackSampler::sKeyDataStart() );
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
	for ( int imrk=0; imrk<mrkrs.size(); imrk++ )
	markernms.addIfNew( mrkrs[imrk]->name() );
    }
    markernms.add( Well::TrackSampler::sKeyDataEnd() );

    for ( int idx=0; idx<lognms.size(); idx++ )
	logsfld_->addItem( lognms.get(idx) );
    topmarkfld_->addItems( markernms );
    botmarkfld_->addItems( markernms );
    topmarkfld_->setCurrentItem( 0 );
    botmarkfld_->setCurrentItem( markernms.size() - 1 );
}


void uiMultiWellLogSel::getSelWellIDs( TypeSet<MultiID>& wids ) const
{
    for ( int idx=0; idx<wellsfld_->size(); idx++ )
    {
	if ( wellsfld_->isSelected(idx) )
	    wids += wellids_[idx];
    }
} 


void uiMultiWellLogSel::getSelWellNames( BufferStringSet& wellnms ) const
{ wellsfld_->getSelectedItems( wellnms ); }


void uiMultiWellLogSel::getSelLogNames( BufferStringSet& lognms ) const
{ logsfld_->getSelectedItems( lognms ); }


