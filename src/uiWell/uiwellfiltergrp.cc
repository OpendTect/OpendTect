/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2019
________________________________________________________________________

-*/

#include "uiwellfiltergrp.h"

#include "welldatafilter.h"

#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uimnemonicsel.h"
#include "uistatusbar.h"
#include "uitoolbutton.h"
#include "welltransl.h"



uiWellFilterGrp::uiWellFilterGrp( uiParent* p, OD::Orientation orient )
    : uiGroup(p)
{
    const bool hor = orient == OD::Horizontal;
    const IOObjContext ctxt = mIOObjContext( Well );
    uiIOObjSelGrp::Setup suw( OD::ChooseZeroOrMore );
    uiIOObjSelGrp* welllistselgrp = new uiIOObjSelGrp( this, ctxt, suw );
    welllistselgrp->displayManipGroup( false, true );
    welllist_ = welllistselgrp->getListField();
    welllist_->chooseAll();
    welllist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    loglist_ = new uiListBox( this, "logs", OD::ChooseZeroOrMore );
    logfilter_ = new uiListBoxFilter( *loglist_ );
    loglist_->setHSzPol( uiObject::Wide );
    loglist_->attach( hor ? rightOf : alignedBelow, welllistselgrp );
    loglist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    markerlist_ = new uiListBox( this, "markers", OD::ChooseZeroOrMore );
    markerfilter_ = new uiListBoxFilter( *markerlist_ );
    markerlist_->attach( hor ? rightOf : alignedBelow, loglist_);
    markerlist_->setHSzPol( uiObject::Wide );
    markerlist_->addLabel( uiStrings::sSelection(), uiListBox::BelowMid );

    CallBack cb = mCB(this,uiWellFilterGrp,selButPush);
    fromwellbut_ = new uiToolButton( this,
		 hor ? uiToolButton::RightArrow : uiToolButton::DownArrow,
		tr("Show logs/markers present selected wells"), cb );
    fromwellbut_->attach(hor ? centeredBelow : centeredRightOf, welllistselgrp);
    fromlogbut_ = new uiToolButton( this,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected logs"), cb );
    fromlogbut_->attach( hor ? centeredBelow : centeredRightOf, loglist_ );
    frommarkerbut_ = new uiToolButton( this,
		hor ? uiToolButton::LeftArrow : uiToolButton::UpArrow,
		tr("Show wells which have selected markers"), cb );
    frommarkerbut_->attach( hor ? centeredBelow : centeredRightOf,
			    markerlist_ );

    mAttachCB( welllistselgrp->selectionChanged, uiWellFilterGrp::selChgCB );
    mAttachCB( loglist_->selectionChanged, uiWellFilterGrp::selChgCB );
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


uiWellFilterGrp::~uiWellFilterGrp()
{
    detachAllNotifiers();
}


void uiWellFilterGrp::setFilterItems( const ObjectSet<Well::Data>& wds,
					   const BufferStringSet& lognms,
					   const BufferStringSet& markernms)
{
    wds_ = &wds;
    BufferStringSet sortedlognms = lognms;
    sortedlognms.sort();
    logfilter_->setItems( sortedlognms );
    loglist_->chooseAll();

    markerfilter_->setItems( markernms );
    markerlist_->chooseAll();

    int maxsz = mMAX( welllist_->size(),
		      mMAX(loglist_->size(),markerlist_->size()) );
    if ( maxsz > 25 )
	maxsz = 25;

    welllist_->setNrLines( maxsz );
    loglist_->setNrLines( maxsz );
    markerlist_->setNrLines( maxsz );
}


void uiWellFilterGrp::setSelected( const BufferStringSet& wellnms,
				   const BufferStringSet& lognms,
				   const BufferStringSet& mrkrnms )
{
    welllist_->setChosen( wellnms );
    loglist_->setChosen( lognms );
    markerlist_->setChosen( mrkrnms );
    selChgCB( nullptr );
}


void uiWellFilterGrp::getSelected( BufferStringSet& wellnms,
				   BufferStringSet& lognms,
				   BufferStringSet& mrkrnms ) const
{
    welllist_->getChosen( wellnms );
    loglist_->getChosen( lognms );
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
    loglist_->setChosen( suitablelogs );
}


void uiWellFilterGrp::selChgCB( CallBacker* )
{
    const int selwells = welllist_->nrChosen();
    const int totalwells = welllist_->size();
    welllist_->setLabelText( tr("Selected Wells %1/%2").arg(selwells).
							arg(totalwells), 0 );

    const int sellogs = loglist_->nrChosen();
    const int totallogs = loglist_->size();
    loglist_->setLabelText( tr("Selected Logs %1/%2").arg(sellogs).
							arg(totallogs), 0 );

    const int selmarkers = markerlist_->nrChosen();
    const int totalmarkers = markerlist_->size();
    markerlist_->setLabelText( tr("Selected Markers %1/%2").arg(selmarkers)
						     .arg(totalmarkers), 0 );
}


void uiWellFilterGrp::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    BufferStringSet wellnames, lognames, markernames;
    Well::WellDataFilter wdf( *wds_ );
    if ( but == fromwellbut_ )
    {
	welllist_->getChosen( wellnames );
	wdf.getMarkersLogsFromWells( wellnames, lognames, markernames );
	loglist_->setChosen( lognames );
	markerlist_->setChosen( markernames );
    }
    else if ( but == fromlogbut_ )
    {
	loglist_->getChosen( lognames );
	wdf.getWellsFromLogs( lognames, wellnames );
	welllist_->setChosen( wellnames );
    }
    else if ( but == frommarkerbut_ )
    {
	markerlist_->getChosen( markernames );
	wdf.getWellsFromMarkers( markernames, wellnames );
	welllist_->setChosen( wellnames );
    }
}
