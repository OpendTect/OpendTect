/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2012
________________________________________________________________________

-*/


#include "uiwellmarkersel.h"

#include "globexpr.h"
#include "keystrs.h"
#include "wellextractdata.h"
#include "wellmanager.h"
#include "wellmarker.h"
#include "welltransl.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiwellsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiseparator.h"


const char* uiWellMarkerSel::sKeyUdfLvl()
		{ return "-"; }
const char* uiWellMarkerSel::sKeyDataStart()
		{ return Well::ExtractParams::sKeyDataStart(); }
const char* uiWellMarkerSel::sKeyDataEnd()
		{ return Well::ExtractParams::sKeyDataEnd(); }



uiWellMarkerSel::Setup::Setup( bool issingle, const uiString& txt )
    : seltxt_(txt)
    , single_(issingle)
    , allowsame_(true)
    , withudf_(true)
    , unordered_(false)
    , middef_(false)
    , mediumsz_(false)
{
    if ( seltxt_ == uiStrings::sNone() )
	seltxt_ = uiString::empty();
    else if ( seltxt_.isEmpty() )
    {
	if ( single_ )
	    seltxt_ = uiStrings::sMarker();
	else
	    seltxt_ = withudf_ ? tr("Selected zone") : tr("Top/bottom");
    }
}


uiWellMarkerSel::uiWellMarkerSel( uiParent* p, const uiWellMarkerSel::Setup& su)
	: uiGroup(p,"Well Marker selection")
	, setup_(su)
	, botfld_(0)
	, mrkSelDone(this)
{
    CallBack mrkselcb( mCB(this,uiWellMarkerSel,mrkSel) );
    uiLabeledComboBox* lcb = 0;
    if ( setup_.seltxt_.isEmpty() )
	topfld_ = new uiComboBox( this, "Top marker" );
    else
    {
	lcb = new uiLabeledComboBox( this, setup_.seltxt_, "Top marker" );
	topfld_ = lcb->box();
    }
    topfld_->selectionChanged.notify( mrkselcb );
    if ( setup_.mediumsz_ )
	topfld_->setHSzPol( uiObject::MedVar );

    if ( !setup_.single_ )
    {
	botfld_ = new uiComboBox( this, "Bottom marker" );
	if ( lcb )
	    botfld_->attach( rightOf, lcb );
	else
	    botfld_->attach( rightOf, topfld_ );
	botfld_->selectionChanged.notify( mrkselcb );
	botfld_->setHSzPol( uiObject::MedVar );
	topfld_->setHSzPol( uiObject::MedVar );
	topfld_->setToolTip( tr("Top of the zone") );
	botfld_->setToolTip( tr("Bottom of the zone") );
    }

    setHAlignObj( topfld_ );
}


void uiWellMarkerSel::fillWithAll()
{
    BufferStringSet nms; TypeSet<Color> colors;
    TypeSet<Well::Marker::ZType> mds;
    Well::MGR().getAllMarkerInfos( nms, colors, mds );
    setMarkers( nms, colors );
}


void uiWellMarkerSel::setMarkers( const Well::MarkerSet& wms )
{
    BufferStringSet nms; TypeSet<Color> colors;
    wms.getNames( nms ); wms.getColors( colors );
    setMarkers( nms, colors );
}


void uiWellMarkerSel::setEmpty()
{
    setMarkers( BufferStringSet(), TypeSet<Color>() );
}


void uiWellMarkerSel::setMarkers( const BufferStringSet& inpnms,
				  const TypeSet<Color>& inpcolors )
{
    BufferStringSet nms; TypeSet<Color> colors;
    if ( setup_.withudf_ )
    {
	nms.add( setup_.single_ ? sKeyUdfLvl() : sKeyDataStart() );
	colors.add( Color::NoColor() );
    }
    nms.add( inpnms, true );
    colors.append( inpcolors );
    while ( colors.size() < nms.size() )
	colors += Color::NoColor();
    if ( !setup_.single_ && setup_.withudf_ )
    {
	nms.add( sKeyDataEnd() );
	colors.add( Color::NoColor() );
    }

    if ( nms.isEmpty() )
	{ topfld_->setEmpty(); if ( botfld_ ) botfld_->setEmpty(); return; }

    const int mid = nms.size() / 2;
    setMarkers( *topfld_, nms, colors );
    if ( setup_.middef_ )
	topfld_->setCurrentItem( mid );

    if ( botfld_ )
    {
	setMarkers( *botfld_, nms, colors );
	int defitm = setup_.middef_ ? mid+1 : botfld_->size() - 1;
	if ( defitm >= botfld_->size() )
	    defitm = botfld_->size() - 1;
	botfld_->setCurrentItem( defitm );
	mrkSel( botfld_ );
    }
}


void uiWellMarkerSel::setMarkers( uiComboBox& cb, const BufferStringSet& nms,
				  const TypeSet<Color>& colors )
{
    BufferString cur( cb.text() );
    NotifyStopper ns( cb.selectionChanged );
    cb.setEmpty();
    cb.addItems( nms );
    for ( int idx=0; idx<cb.size(); idx++ )
	cb.setColorIcon( idx, colors[idx] );
    if ( cur.isEmpty() )
	cb.setCurrentItem( 0 );
    else
	cb.setCurrentItem( cur );
}


void uiWellMarkerSel::setInput( const Well::Marker& wm, bool top )
{
    setInput( wm.name(), top );
}


void uiWellMarkerSel::setInput( const char* nm, bool top )
{
    if ( !top && !botfld_ ) return;
    uiComboBox& cb = top ? *topfld_ : *botfld_;
    cb.setCurrentItem( nm );
}


const char* uiWellMarkerSel::getText( bool top ) const
{
    if ( !top && !botfld_ ) return sKeyDataEnd();
    uiComboBox& cb = top ? *topfld_ : *botfld_;
    return cb.text();
}


int uiWellMarkerSel::getType( bool top ) const
{
    const BufferString txt( getText(top) );
    if ( txt == sKeyUdfLvl() || txt == sKeyDataStart() )
	return -1;
    if ( txt == sKeyDataEnd() )
	return 1;
    return 0;
}


void uiWellMarkerSel::usePar( const IOPar& iop )
{
    if ( !botfld_ )
    {
	const char* res = iop.find( sKey::Marker() );
	if ( res && *res )
	    setInput( res, true );
    }
    else
    {
	const char* res = iop.find( Well::ExtractParams::sKeyTopMrk() );
	if ( res && *res )
	    setInput( res, true );
	res = iop.find( Well::ExtractParams::sKeyBotMrk() );
	if ( res && *res )
	    setInput( res, false );
    }
}


void uiWellMarkerSel::fillPar( IOPar& iop ) const
{
    if ( !botfld_ )
	iop.set( sKey::Marker(), getText(true) );
    else
    {
	iop.set( Well::ExtractParams::sKeyTopMrk(), getText(true) );
	if ( botfld_ )
	    iop.set( Well::ExtractParams::sKeyBotMrk(), getText(false) );
    }
}


void uiWellMarkerSel::mrkSel( CallBacker* callingcb )
{
    if ( setup_.single_ || topfld_->size() < 2 || setup_.unordered_ )
	return;

    const bool istop = callingcb == topfld_;
    uiComboBox& cb = istop ? *topfld_ : *botfld_;
    uiComboBox& othcb = !istop ? *topfld_ : *botfld_;
    NotifyStopper ns( cb.selectionChanged );
    NotifyStopper othns( othcb.selectionChanged );
    const int selidx = cb.currentItem();
    int othselidx = othcb.currentItem();
    const int sz = cb.size();

    if ( (istop && selidx > othselidx) || (!istop && selidx < othselidx) )
    {
	othcb.setCurrentItem( selidx );
	othselidx = selidx;
    }

    if ( selidx == othselidx && !setup_.allowsame_ )
    {
	if ( istop )
	{
	    if ( selidx > sz-2 )
		cb.setCurrentItem( sz-2 );
	    else
		othcb.setCurrentItem( selidx+1 );
	}
	else
	{
	    if ( selidx < 1 )
		cb.setCurrentItem( 1 );
	    else
		othcb.setCurrentItem( selidx-1 );
	}
    }
    mrkSelDone.trigger();
}


uiWellMarkersDlg::uiWellMarkersDlg( uiParent* p,
				    const uiWellMarkersDlg::Setup& su )
    : uiDialog(p,uiDialog::Setup(tr("Select well markers"),
		isMultiChoice( su.markerschoicemode_ )
		    ? tr("Select markers from one or more wells")
		    : tr("Select a well marker"),
		mODHelpKey(mWellMarkersDlgHelpID)))
{
    uiGroup* mrkrgrp = new uiGroup( this, "Marker group" );
    uiLabel* markerstxt =
		new uiLabel( mrkrgrp, uiStrings::sMarker(mPlural) );

    markersselgrp_ = new uiListBox( mrkrgrp, "Markers", su.markerschoicemode_ );
    TypeSet<Color> colors;
    TypeSet<Well::Marker::ZType> mds;
    Well::MGR().getAllMarkerInfos( markernms_, colors, mds );
    markersselgrp_->addItems( markernms_ );
    for ( int idx=0; idx<markernms_.size(); idx++ )
	markersselgrp_->setColorIcon( idx, colors[idx] );
    filtfld_ = new uiGenInput( markersselgrp_, uiStrings::sFilter(), "*" );
    filtfld_->updateRequested.notify(
				mCB(this,uiWellMarkersDlg,fltrMarkerNamesCB) );
    markersselgrp_->box()->attach( centeredBelow, filtfld_ );
    markersselgrp_->attach( ensureRightOf, markerstxt );
    mrkrgrp->setHAlignObj( markersselgrp_ );

    if ( !su.withwellfilter_ )
	return;

    uiSeparator* sep = new uiSeparator( this, "Well to markers" );
    sep->attach( stretchedBelow, mrkrgrp );

    uiIOObjSelGrp::Setup wssu( su.wellschoicemode_ );
    wellselgrp_ = new uiMultiWellSel( this, false, &wssu );
    wellselgrp_->attach( alignedBelow, mrkrgrp );
    wellselgrp_->attach( ensureBelow, sep );

    uiLabel* txt = new uiLabel( this, uiStrings::sWells() );
    txt->attach( leftBorder );
    txt->attach( ensureBelow, sep );
}


void uiWellMarkersDlg::getNames( BufferStringSet& markernms )
{
    markersselgrp_->getChosen( markernms );
}


void uiWellMarkersDlg::getWellNames( BufferStringSet& wllnms )
{
    if ( wellselgrp_ )
    {
	DBKeySet wllids;
	wellselgrp_->getSelected( wllids );
	for ( int idx=0; idx<wllids.size(); idx++ )
	    wllnms.add( Well::MGR().nameOf( wllids[idx] ) );
    }
}


void uiWellMarkersDlg::getWellIDs( DBKeySet& wllids )
{
    if ( wellselgrp_ )
	wellselgrp_->getSelected( wllids );
}


void uiWellMarkersDlg::fltrMarkerNamesCB( CallBacker* )
{
    markersselgrp_->setEmpty();
    if ( markernms_.isEmpty() )
	return;

    BufferString filtstr = filtfld_->text();
    if ( filtstr.isEmpty() || filtstr == "*" )
    {
	markersselgrp_->addItems( markernms_ );
	return;
    }

    const int mrkrsz = markernms_.size();
    GlobExpr ge( filtstr );
    BufferStringSet filtmrkrnms;
    for ( int midx=0; midx<mrkrsz; midx++ )
    {
	BufferString mrkrnm = markernms_.get( midx );
	if ( !ge.matches(mrkrnm) )
		continue;

	filtmrkrnms.add( mrkrnm );
    }

    if ( filtmrkrnms.isEmpty() )
	return;

    markersselgrp_->addItems( filtmrkrnms );
}
