/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiwellmarkersel.cc,v 1.1 2012-08-27 11:45:36 cvsbert Exp $";


#include "uiwellmarkersel.h"

#include "uicombobox.h"
#include "wellmarker.h"

uiWellMarkerSel::Setup::Setup( bool issingle, const char* txt )
    : seltxt_("Marker")
    , single_(issingle)
    , allowsame_(false)
    , adddatabounds_(false)
{
    if ( !single_ )
	seltxt_ = adddatabounds_ ? "Selected zone" : "Top/bottom";
}


uiWellMarkerSel::uiWellMarkerSel( uiParent* p, const uiWellMarkerSel::Setup& su)
	: uiGroup(p,"Well Marker selection")
    	, setup_(su)
    	, botfld_(0)
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

    if ( !setup_.single_ )
    {
	botfld_ = new uiComboBox( this, "Bottom marker" );
	if ( lcb )
	    botfld_->attach( rightOf, lcb );
	else
	    botfld_->attach( rightOf, topfld_ );
	botfld_->selectionChanged.notify( mrkselcb );
    }
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
    if ( setup_.adddatabounds_ ) nms.add( sKeyDataStart() );
    nms.add( inpnms, true );
    if ( setup_.adddatabounds_ ) nms.add( sKeyDataStop() );
    setMarkers( *topfld_, nms );
    if ( botfld_ )
    {
	setMarkers( *botfld_, nms );
	mrkSel( botfld_ );
    }
}


void uiWellMarkerSel::setMarkers( uiComboBox& cb, const BufferStringSet& nms )
{
    BufferString cur( cb.text() );
    NotifyStopper ns( cb.selectionChanged );
    cb.setEmpty();
    cb.addItems( nms );
    if ( !cur.isEmpty() )
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
    if ( !top && !botfld_ ) return sKeyDataStop();
    uiComboBox& cb = top ? *topfld_ : *botfld_;
    return cb.text();
}


int uiWellMarkerSel::getType( bool top ) const
{
    BufferString txt( getText(top) );
    if ( *txt.buf() != '<' ) return 0;
    if ( txt == sKeyDataStart() ) return -1;
    if ( txt == sKeyDataStop() ) return 1;
    return 0;
}


void uiWellMarkerSel::mrkSel( CallBacker* callingcb )
{
    if ( !botfld_ || topfld_->size() < 2 )
	return;

    const bool istop = callingcb == topfld_;
    uiComboBox& cb = istop ? *topfld_ : *botfld_;
    uiComboBox& othcb = !istop ? *topfld_ : *botfld_;
    NotifyStopper ns( cb.selectionChanged );
    NotifyStopper othns( othcb.selectionChanged );
    const int selidx = cb.currentItem();
    int othselidx = othcb.currentItem();
    const int sz = cb.size();

    if ( (istop && selidx > othselidx)
      || (!istop && selidx < othselidx) )
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
		othcb.setCurrentItem( selidx-1 );
	}
	else
	{
	    if ( selidx < 1 )
		cb.setCurrentItem( 1 );
	    else
		othcb.setCurrentItem( selidx-1 );
	}
    }
}
