/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2019
________________________________________________________________________

-*/

#include "uiwellfiltergrp.h"

#include "arrayndimpl.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welltransl.h"

#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uimnemonicsel.h"
#include "uistatusbar.h"
#include "uitoolbutton.h"


// WellDataFilter
WellDataFilter::WellDataFilter( const ObjectSet<Well::Data>& wds )
    : allwds_(wds)
{}


WellDataFilter::~WellDataFilter()
{}


void WellDataFilter::getWellsFromLogs( const BufferStringSet& lognms,
				       BufferStringSet& wellnms ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const Well::LogSet& lis = wd->logs();
	BufferStringSet wdlognms; lis.getNames( wdlognms );
	bool addwell = true;
	for ( int lidx=0; lidx<lognms.size(); lidx++ )
	{
	    const bool wdhaslog = wdlognms.isPresent( lognms.get(lidx) );
	    if ( wdhaslog )
		continue;

	    addwell = false;
	    break;
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsWithNoLogs( BufferStringSet& wellnms ) const
{
    for ( const auto* wd : allwds_ )
    {
	if ( wd->logs().isEmpty() )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getWellsFromMarkers( const BufferStringSet& markernms,
					  BufferStringSet& wellnms ) const
{
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const Well::MarkerSet& ms = wd->markers();
	BufferStringSet wdmarkernms; ms.getNames( wdmarkernms );
	bool addwell = true;
	for ( int midx=0; midx<markernms.size(); midx++ )
	{
	    const BufferString& markernm = markernms.get( midx );
	    const bool wdhasmrkr =
			markernm==Well::ZRangeSelector::sKeyDataStart() ||
			markernm==Well::ZRangeSelector::sKeyDataEnd() ||
			wdmarkernms.isPresent( markernm );
	    if ( wdhasmrkr )
		continue;

	    addwell = false;
	    break;
	}

	if ( addwell )
	    wellnms.add( wd->name() );
    }
}


void WellDataFilter::getMarkersLogsFromWells( const BufferStringSet& wellnms,
					      BufferStringSet& lognms,
					      BufferStringSet& markernms ) const
{
    bool first = true;
    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	if ( first )
	{
	    wd->logs().getNames( lognms );
	    wd->markers().getNames( markernms );
	    first = false;
	}
	else
	{
	    BufferStringSet wdlognms, wdmarkernms, lognms2rm, markernms2rm;
	    wd->logs().getNames( wdlognms );
	    wd->markers().getNames( wdmarkernms );
	    for ( int lidx=0; lidx<lognms.size(); lidx++ )
	    {
		const BufferString& lognm = lognms.get( lidx );
		if ( wdlognms.isPresent(lognm) )
		    continue;

		lognms.removeSingle( lidx );
		lidx--;
	    }

	    for ( int midx=0; midx<markernms.size(); midx++ )
	    {
		const BufferString& markernm = markernms.get( midx );
		if ( wdmarkernms.isPresent(markernm) )
		    continue;

		markernms.removeSingle( midx );
		midx--;
	    }
	}
    }
}


void WellDataFilter::getLogPresence( const BufferStringSet& wellnms,
				     const char* topnm, const char* botnm,
				     const BufferStringSet& alllognms,
				     Array2D<int>& presence,
				     BufferStringSet& lognms,
				     Well::Info::DepthType depthtype ) const
{
    presence.setAll( -1 );

    for ( int widx=0; widx<allwds_.size(); widx++ )
    {
	const Well::Data* wd = allwds_[widx];
	const bool haswellnm = wellnms.isPresent( wd->name() );
	if ( !haswellnm )
	    continue;

	Interval<float> markerrg = Interval<float>::udf();
	float kbelv = wd->track().getKbElev();
	if ( FixedString(topnm) == Well::ZRangeSelector::sKeyDataStart() )
	    markerrg.start = wd->track().dahRange().start;
	else
	{
	    const Well::Marker* marker = wd->markers().getByName( topnm );
	    if ( marker )
	    {
		float mrkrdahstart = marker->dah();
		if ( depthtype == Well::Info::MD)
		    markerrg.start = mrkrdahstart;
		else
		{
		    markerrg.start = wd->track().getPos(mrkrdahstart).z;
		    if ( depthtype == Well::Info::TVD )
			markerrg.start += kbelv;
		}
	    }
	}

	if ( FixedString(botnm) == Well::ZRangeSelector::sKeyDataEnd() )
	    markerrg.stop = wd->track().dahRange().stop;
	else
	{
	    const Well::Marker* marker = wd->markers().getByName( botnm );
	    if ( marker )
	    {
		float mrkrdahstop = marker->dah();
		if ( depthtype == Well::Info::MD )
		    markerrg.stop = mrkrdahstop;
		else
		{
		    markerrg.stop = wd->track().getPos(mrkrdahstop).z;
		    if ( depthtype == Well::Info::TVD )
			markerrg.stop += kbelv;
		}
	    }
	}

	if ( markerrg.isUdf() )
	{
	    for ( int lidx=0; lidx<alllognms.size(); lidx++ )
		presence.set( widx, lidx, 100 );
	    continue;
	}

	const float markerwidth = markerrg.width();
	for ( int lidx=0; lidx<alllognms.size(); lidx++ )
	{
	    const BufferString& lognm = alllognms.get( lidx );
	    const Well::Log* log = wd->logs().getLog( lognm );
	    if ( !log )
		continue;

	    const Interval<float>& logrg = log->dahRange();
	    int perc = 0;
	    if ( logrg.includes(markerrg,false) )
		perc = 100;
	    else if ( !logrg.overlaps(markerrg,false) )
		perc = 0;
	    else
	    {
		Interval<float> avlogrg = logrg;
		avlogrg.limitTo( markerrg );
		perc = mNINT32( 100.f * avlogrg.width()/markerwidth );
	    }

	    presence.set( widx, lidx, perc );
	}
    }
}


void WellDataFilter::getLogsForMnems(const MnemonicSelection& mns,
				     BufferStringSet& lognms) const
{
    for ( const auto* wd : allwds_ )
    {
	for ( const auto* mn : mns )
	{
	    TypeSet<int> idxs = wd->logs().getSuitable( *mn );
	    for ( const auto& idx : idxs )
		lognms.addIfNew( wd->logs().getLog(idx).name() );
	}
    }
}


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
    WellDataFilter wdf( *wds_ );
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

    WellDataFilter wdf( *wds_ );
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
    WellDataFilter wdf( *wds_ );
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


void uiWellFilterGrp::toStatusBar( const uiString& uistr, int fldidx,
					int msecs )
{
    if ( !statusbar_ )
	return;

    statusbar_->message( uistr, fldidx, msecs );
}
