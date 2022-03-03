/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2019
________________________________________________________________________

-*/

#include "uiwellfiltergrp.h"

#include "welldatafilter.h"

#include "ioman.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uimnemonicsel.h"
#include "uistatusbar.h"
#include "uitoolbutton.h"
#include "wellman.h"
#include "welltransl.h"



uiWellFilterGrp::uiWellFilterGrp( uiParent* p, OD::Orientation orient )
    : uiGroup(p)
    , orient_(orient)
{
    const bool hor = orient_ == OD::Horizontal;
    const IOObjContext ctxt = mIOObjContext( Well );
    uiIOObjSelGrp::Setup suw( OD::ChooseZeroOrMore );
    uiIOObjSelGrp* welllistselgrp = new uiIOObjSelGrp( this, ctxt, suw );
    welllistselgrp->displayManipGroup( false, true );
    welllist_ = welllistselgrp->getListField();
    welllist_->chooseAll();
    welllist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    logormnslist_ = new uiListBox( this, "logs", OD::ChooseZeroOrMore );
    logormnsfilter_ = new uiListBoxFilter( *logormnslist_ );
    logormnslist_->setHSzPol( uiObject::Wide );
    logormnslist_->attach( hor ? rightOf : alignedBelow, welllistselgrp );
    logormnslist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    markerlist_ = new uiListBox( this, "markers", OD::ChooseZeroOrMore );
    markerfilter_ = new uiListBoxFilter( *markerlist_ );
    markerlist_->attach( hor ? rightOf : alignedBelow, logormnslist_ );
    markerlist_->setHSzPol( uiObject::Wide );
    markerlist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    CallBack cb = mCB(this,uiWellFilterGrp,selButPush);
    fromwellbut_ = new uiToolButton( this,
		 hor ? uiToolButton::RightArrow : uiToolButton::DownArrow,
		tr("Show logs/markers present selected wells"), cb );
    fromwellbut_->attach(hor ? centeredBelow : centeredRightOf, welllistselgrp);
    fromlogormnsbut_ = new uiToolButton( this,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected logs/mnemonics"), cb );
    fromlogormnsbut_->attach( hor ? centeredBelow : centeredRightOf, 
	    		      logormnslist_ );
    frommarkerbut_ = new uiToolButton( this,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected markers"), cb );
    frommarkerbut_->attach( hor ? centeredBelow : centeredRightOf,
			    markerlist_ );

    mAttachCB( welllistselgrp->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( logormnslist_->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( markerlist_->selectionChanged, uiWellFilterGrp::selChgCB );
}


uiWellFilterGrp::uiWellFilterGrp( uiParent* p, const ObjectSet<Well::Data>& wds,
				  const BufferStringSet& lognms,
				  const BufferStringSet& markernms,
				  OD::Orientation orient )
    : uiWellFilterGrp(p, orient)
{
    setFilterItems( wds, lognms, markernms );
}


uiWellFilterGrp::uiWellFilterGrp( uiParent* p, const ObjectSet<Well::Data>& wds,
				  const MnemonicSelection& mns,
				  const BufferStringSet& markernms,
				  OD::Orientation orient )
    : uiWellFilterGrp(p, orient)
{
    setFilterItems( wds, mns, markernms );
    logmode_ = false;
}


uiWellFilterGrp::~uiWellFilterGrp()
{
    detachAllNotifiers();
}


void uiWellFilterGrp::setFilterItems( const ObjectSet<Well::Data>& wds,
				      const BufferStringSet& lognms,
				      const BufferStringSet& markernms )
{
    wds_ = &wds;
    BufferStringSet sortedlognms = lognms;
    sortedlognms.sort();
    logormnsfilter_->setItems( sortedlognms );
    logormnslist_->chooseAll();
    markerfilter_->setItems( markernms );
    markerlist_->chooseAll();
    setMaxLinesForLists();
}


void uiWellFilterGrp::setFilterItems( const ObjectSet<Well::Data>& wds,
				      const MnemonicSelection& mns,
				      const BufferStringSet& markernms )
{
    wds_ = &wds;
    mns_ = &mns;
    if ( !mns.isEmpty() )
    {
	BufferStringSet mnemnms;
	for ( const auto* mn : mns )
	    mnemnms.addIfNew( mn->name() );

	logormnsfilter_->setItems( mnemnms );
	logormnslist_->chooseAll();
    }

    markerfilter_->setItems( markernms );
    markerlist_->chooseAll();
    setMaxLinesForLists();
}


void uiWellFilterGrp::setMaxLinesForLists()
{
    int maxsz = mMAX( welllist_->size(),
		      mMAX(logormnslist_->size(),markerlist_->size()) );
    if ( maxsz > 25 )
	maxsz = 25;

    welllist_->setNrLines( maxsz );
    logormnslist_->setNrLines( maxsz );
    markerlist_->setNrLines( maxsz );
}


void uiWellFilterGrp::setSelected( const DBKeySet& wellids,
				   const BufferStringSet& lognms,
				   const BufferStringSet& mrkrnms )
{
    BufferStringSet wellnms;
    for ( const auto* wellid : wellids )
	wellnms.add( IOM().objectName(*wellid) );

    welllist_->setChosen( wellnms );
    logormnslist_->setChosen( lognms );
    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}

void uiWellFilterGrp::setSelected( const BufferStringSet& wellnms,
				   const BufferStringSet& lognms,
				   const BufferStringSet& mrkrnms )
{
    welllist_->setChosen( wellnms );
    logormnslist_->setChosen( lognms );
    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}


void uiWellFilterGrp::setSelected( const BufferStringSet& wellnms,
				   const MnemonicSelection& mns,
				   const BufferStringSet& mrkrnms )
{
    welllist_->setChosen( wellnms );
    if ( !mns.isEmpty() )
    {
	BufferStringSet mnemnms;
	for ( const auto* mn : mns )
	    mnemnms.addIfNew( mn->name() );

	logormnslist_->setChosen( mnemnms );
    }

    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}


void uiWellFilterGrp::getSelected( DBKeySet& wellids,
				   BufferStringSet& lognms,
				   BufferStringSet& mrkrnms ) const
{
    BufferStringSet wellnms;
    welllist_->getChosen( wellnms );
    logormnslist_->getChosen( lognms );
    markerlist_->getChosen( mrkrnms );
    wellids.setEmpty();
    for ( const auto* wellnm : wellnms )
    {
	const IOObj* ioobj = Well::findIOObj( *wellnm, nullptr );
	if ( !ioobj )
	    continue;
	wellids += ioobj->key();
    }
}


void uiWellFilterGrp::getSelected( BufferStringSet& wellnms,
				   BufferStringSet& lognms,
				   BufferStringSet& mrkrnms ) const
{
    welllist_->getChosen( wellnms );
    logormnslist_->getChosen( lognms );
    markerlist_->getChosen( mrkrnms );
}


void uiWellFilterGrp::getSelected( BufferStringSet& wellnms,
				   MnemonicSelection& mns,
				   BufferStringSet& mrkrnms ) const
{
    welllist_->getChosen( wellnms );
    mns.setEmpty();
    BufferStringSet selmnnms;
    logormnslist_->getChosen( selmnnms );
    for ( const auto* mnnm : selmnnms )
	mns.addIfNew( mns_->getByName(*mnnm) );

    markerlist_->getChosen( mrkrnms );
}


void uiWellFilterGrp::noLogFilterCB( CallBacker* )
{
    BufferStringSet wellstohide;
    TypeSet<int> idxstohide;
    Well::WellDataFilter wdf( *wds_ );
    wdf.getWellsWithNoLogs( wellstohide );
    for ( const auto* wellnm : wellstohide )
	idxstohide += welllist_->indexOf( *wellnm );

    for ( const auto& idx : idxstohide )
	welllist_->setChosen( idx, false );
}


void uiWellFilterGrp::mnemFilterCB( CallBacker* )
{
    MnemonicSelection mns;
    BufferStringSet suitablelogs;
    uiMultiMnemonicsSel dlg( this, mns );
    if ( !dlg.go() )
	return;

    Well::WellDataFilter wdf( *wds_ );
    wdf.getLogsForMnems( mns, suitablelogs );
    logormnslist_->setChosen( suitablelogs );
}


void uiWellFilterGrp::selChgCB( CallBacker* )
{
    const int selwells = welllist_->nrChosen();
    const int totalwells = welllist_->size();
    welllist_->setLabelText( tr("Selected Wells %1/%2").arg(selwells).
							arg(totalwells), 0 );

    const char* logormn = logmode_ ? "Logs" : "Mnemonics";
    const int sellogsormns = logormnslist_->nrChosen();
    const int totallogsormns =	logormnslist_->size();
    logormnslist_->setLabelText( tr("Selected %1 %2/%3").arg(logormn)
						    .arg(sellogsormns)
						    .arg(totallogsormns), 0 );

    const int selmarkers = markerlist_->nrChosen();
    const int totalmarkers = markerlist_->size();
    markerlist_->setLabelText( tr("Selected Markers %1/%2").arg(selmarkers)
						     .arg(totalmarkers), 0 );
}


void uiWellFilterGrp::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    BufferStringSet wellnames, lognames, markernames;
    MnemonicSelection mns;
    Well::WellDataFilter wdf( *wds_ );
    if ( but == fromwellbut_ )
    {
	welllist_->getChosen( wellnames );
	wdf.getMarkersLogsMnemsFromWells( wellnames,
					  lognames, mns, markernames );
	if ( logmode_ )
	    logormnslist_->setChosen( lognames );
	else
	{
	    BufferStringSet mnnms;
	    for ( const auto* mn : mns )
		mnnms.add( mn->name() );

	    logormnslist_->setChosen( mnnms );
	}

	markerlist_->setChosen( markernames );
    }
    else if ( but == fromlogormnsbut_ )
    {
	if ( logmode_ )
	{
	    logormnslist_->getChosen( lognames );
	    wdf.getWellsFromLogs( lognames, wellnames );
	}
	else
	{
	    BufferStringSet mnnms;
	    logormnslist_->getChosen( mnnms );
	    for ( const auto* mnnm : mnnms )
		mns.addIfNew( mns_->getByName(*mnnm) );

	    wdf.getWellsFromMnems( mns, wellnames );
	}

	welllist_->setChosen( wellnames );
    }
    else if ( but == frommarkerbut_ )
    {
	markerlist_->getChosen( markernames );
	wdf.getWellsFromMarkers( markernames, wellnames );
	welllist_->setChosen( wellnames );
    }
}
