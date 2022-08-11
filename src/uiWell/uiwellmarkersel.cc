/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2012
________________________________________________________________________

-*/


#include "uiwellmarkersel.h"

#include "globexpr.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "wellman.h"
#include "wellmarker.h"
#include "welltransl.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiseparator.h"


mDefineEnumUtils( uiWellMarkerSel, MarkerSelTyp, "Marker selection type" )
{
    "-",
    Well::ExtractParams::sKeyDataStart(),
    Well::ExtractParams::sKeyDataEnd(),
    "Marker",
    nullptr
};


// uiWellMarkerSel::Setup

uiWellMarkerSel::Setup::Setup( bool issingle, const uiString& txt )
    : seltxt_(txt)
    , single_(issingle)
    , allowsame_(true)
    , withudf_(true)
    , unordered_(false)
    , middef_(false)
{
    if ( txt.isEmpty() )
    {
	if ( single_ )
	    seltxt_ = uiStrings::sMarker();
	else
	    seltxt_ = withudf_ ? tr("Selected zone") : tr("Top/bottom");
    }
}


// uiWellMarkerSel

uiWellMarkerSel::uiWellMarkerSel( uiParent* p, const uiWellMarkerSel::Setup& su)
    : uiGroup(p,"Well Marker selection")
    , setup_(su)
    , mrkSelDone(this)
{
    CallBack mrkselcb( mCB(this,uiWellMarkerSel,mrkSel) );
    uiLabeledComboBox* lcb = nullptr;
    if ( setup_.seltxt_.isEmpty() )
	topfld_ = new uiComboBox( this, "Top marker" );
    else
    {
	lcb = new uiLabeledComboBox( this, setup_.seltxt_, "Top marker" );
	topfld_ = lcb->box();
    }

    topfld_->setStretch( 0, 0 );
    topfld_->setHSzPol( uiObject::Medium );
    topfld_->setToolTip( tr("Top of the zone") );
    topfld_->selectionChanged.notify( mrkselcb );

    if ( !setup_.single_ )
    {
	botfld_ = new uiComboBox( this, "Bottom marker" );
	if ( lcb )
	    botfld_->attach( rightOf, lcb );
	else
	    botfld_->attach( rightOf, topfld_ );

	botfld_->setStretch( 0, 0 );
	botfld_->setHSzPol( uiObject::Medium );
	botfld_->setToolTip( tr("Bottom of the zone") );
	botfld_->selectionChanged.notify( mrkselcb );
    }

    setHAlignObj( topfld_ );
}


void uiWellMarkerSel::setMarkers( const Well::MarkerSet& wms )
{
    BufferStringSet nms;
    for ( int idx=0; idx<wms.size(); idx++ )
	nms.add( wms[idx]->name() );
    setMarkers( nms );
}


void uiWellMarkerSel::setMarkers( const BufferStringSet& inpnms )
{
    BufferStringSet nms;
    if ( setup_.withudf_ )
	nms.add( setup_.single_ ? MarkerSelTypDef().getKey( Undef )
				: MarkerSelTypDef().getKey( Start ) );
    nms.add( inpnms, true );
    if ( !setup_.single_ && setup_.withudf_ )
	nms.add( MarkerSelTypDef().getKey( End ) );
    if ( nms.isEmpty() )
	{ topfld_->setEmpty(); if ( botfld_ ) botfld_->setEmpty(); return; }

    const int mid = nms.size() / 2;
    setMarkers( *topfld_, nms );
    if ( setup_.middef_ )
	topfld_->setCurrentItem( mid );

    if ( botfld_ )
    {
	setMarkers( *botfld_, nms );
	int defitm = setup_.middef_ ? mid+1 : botfld_->size() - 1;
	if ( defitm >= botfld_->size() )
	    defitm = botfld_->size() - 1;
	botfld_->setCurrentItem( defitm );
	mrkSel( botfld_ );
    }
}


void uiWellMarkerSel::setMarkerColors( const TypeSet<OD::Color>& cols )
{
    TypeSet<OD::Color> colors;
    if ( setup_.withudf_ )
	colors += OD::Color::NoColor();

    colors.append( cols );

    for ( int idx=0; idx<colors.size(); idx++ )
    {
	if ( topfld_ )
	    topfld_->setColorIcon( idx, colors[idx] );

	if ( botfld_ )
	    botfld_->setColorIcon( idx, colors[idx] );
    }
}


void uiWellMarkerSel::setMarkers( uiComboBox& cb, const BufferStringSet& nms )
{
    BufferString cur( cb.text() );
    NotifyStopper ns( cb.selectionChanged );
    cb.setEmpty();
    cb.addItems( nms );
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
    if ( !top && !botfld_ ) return MarkerSelTypDef().getKey( End );
    uiComboBox& cb = top ? *topfld_ : *botfld_;
    return cb.text();
}


BufferString uiWellMarkerSel::getMarkerName( bool top ) const
{
    return getMkType(top) == Marker ? getText( top ) : "";
}


uiWellMarkerSel::MarkerSelTyp uiWellMarkerSel::getMkType( bool top ) const
{
    const BufferString txt( getText(top) );
    if ( txt == MarkerSelTypDef().getKey(Undef) )
	return Undef;
    else if ( txt == MarkerSelTypDef().getKey(Start) )
	return Start;
    else if ( txt == MarkerSelTypDef().getKey(End) )
	return End;

    return Marker;
}


int uiWellMarkerSel::getType( bool top ) const
{
    const MarkerSelTyp typ = getMkType( top );
    if ( typ == Undef || typ == Start )
	return -1;
    if ( typ == End )
	return 1;
    return 0;
}


bool uiWellMarkerSel::isValidMarker( bool top ) const
{
    return getMkType( top ) == Marker;
}


void uiWellMarkerSel::reset()
{
    topfld_->setCurrentItem( 0 );
    botfld_->setCurrentItem( botfld_->size()-1 );
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


uiRetVal uiWellMarkerSel::isOK() const
{
    uiRetVal uirv;
    if ( getMkType(true) == End || (botfld_ && getMkType(false) == Start) )
	uirv.add( tr("Invalid markers selection") );
    if ( !setup_.single_ && !setup_.allowsame_ &&
	 StringView(getText(true)) == StringView(getText(false)) )
	uirv.add( tr("Start and stop markers need to be different") );

    return uirv;
}


void uiWellMarkerSel::fillPar( IOPar& iop, bool replacestartend ) const
{
    if ( !botfld_ )
	iop.set( sKey::Marker(), replacestartend ? getMarkerName().buf()
						 : getText() );
    else
    {
	iop.set( Well::ExtractParams::sKeyTopMrk(),
		replacestartend ? getMarkerName(true).buf() : getText(true) );
	if ( botfld_ )
	    iop.set( Well::ExtractParams::sKeyBotMrk(),
	      replacestartend ? getMarkerName(false).buf() : getText(false) );
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
    BufferStringSet markernms;
    Well::MGR().getAllMarkerNames( markernms );
    markernms.sort();
    markersselgrp_->addItems( markernms );

    uiGenInput* filtfld = new uiGenInput( markersselgrp_,
					  uiStrings::sFilter(), "*" );
    filtfld->updateRequested.notify(
				mCB(this,uiWellMarkersDlg,fltrMarkerNamesCB) );
    markersselgrp_->box()->attach( centeredBelow, filtfld );
    markersselgrp_->attach( ensureRightOf, markerstxt );
    mrkrgrp->setHAlignObj( markersselgrp_ );

    if ( !su.withwellfilter_ )
	return;

    uiSeparator* sep = new uiSeparator( this, "Well to markers" );
    sep->attach( stretchedBelow, mrkrgrp );

    wellselgrp_ = new uiIOObjSelGrp( this, mIOObjContext(Well),
				uiIOObjSelGrp::Setup(su.wellschoicemode_) );
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
	wellselgrp_->getChosen( wllnms );

}


void uiWellMarkersDlg::getWellIDs( TypeSet<MultiID>& wllids )
{
    if ( wellselgrp_ )
	wellselgrp_->getChosen( wllids );
}


void uiWellMarkersDlg::fltrMarkerNamesCB( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,filtfld,cb);
    if ( !filtfld )
	return;

    markersselgrp_->setEmpty();
    BufferStringSet markernms;
    Well::MGR().getAllMarkerNames( markernms );
    if ( markernms.isEmpty() )
	return;

    BufferString filtstr = filtfld->text();
    if ( filtstr.isEmpty() || filtstr == "*" )
    {
	markernms.sort();
	markersselgrp_->addItems( markernms );
	return;
    }

    const int mrkrsz = markernms.size();
    GlobExpr ge( filtstr );
    BufferStringSet filtmrkrnms;
    for ( int midx=0; midx<mrkrsz; midx++ )
    {
	BufferString mrkrnm = markernms.get( midx );
	if ( !ge.matches(mrkrnm) )
		continue;

	filtmrkrnms.add( mrkrnm );
    }

    if ( filtmrkrnms.isEmpty() )
	return;

    filtmrkrnms.sort();
    markersselgrp_->addItems( filtmrkrnms );
}
