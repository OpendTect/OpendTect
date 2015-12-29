/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "picksettr.h"
#include "pickset.h"
#include "keystrs.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)


uiPickSetMan::uiPickSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( toUiString("%1/%2")
                .arg( uiStrings::sPickSet(mPlural) )
                .arg( uiStrings::sPolygon(mPlural))),
             mNoDlgTitle,
             mODHelpKey(mPickSetManHelpID) )
                 .nrstatusflds(1).modal(false),
	           PickSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    mergebut_ = selgrp_->getManipGroup()->addButton( "mergepicksets",
				    uiStrings::phrMerge(uiStrings::sPickSet()),
				    mCB(this,uiPickSetMan,mergeSets) );
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    uiString tt;
    if ( chsnnms.size() > 1 )
	tt = uiStrings::phrMerge( toUiString(chsnnms.getDispString(2)) );
    else
	tt = uiStrings::phrMerge(uiStrings::sPickSet());

    mergebut_->setToolTip( tt );
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    Pick::Set ps;
    uiString errmsg;
    if ( !PickSetTranslator::retrieve(ps,curioobj_,true,errmsg) )
    {
	BufferString msg( "Read error: '" );
	msg.add( errmsg.getFullString() );
	msg.add( "'" ).add( "\n<No specific info available>\n" );
    }
    else
    {
	if ( !txt.isEmpty() )
	    ErrMsg( txt );

	FixedString typ = curioobj_->pars().find( sKey::Type() );
	const bool ispoly = typ==sKey::Polygon();
	const bool havetype = typ && *typ;
	if ( havetype )
	    txt.add( "Type: " ).add( typ );

	const int sz = ps.size();
	if ( sz < 1 )
	    txt.add( havetype ? " <empty>" : "Empty Pick Set." );
	else
	{
	    txt.add( havetype ? " <" : "Size: " );
	    txt.add( sz );
	    if ( havetype )
	    {
		txt.add( ispoly ? " vertice" : " pick" );
		if ( sz > 1 )
		    txt += "s";
	    }
	    if ( !ispoly && ps[0].hasDir() )
		txt += " (with directions)";

	    if ( ispoly && sz > 2 )
	    {
		const float area = ps.getXYArea();
		if ( !mIsUdf(area) )
		    txt.add( ", area=" ).add( area );
	    }

	    if ( havetype )
		txt += ">";
	}

	Color col( ps.disp_.color_ ); col.setTransparency( 0 );
	txt.add( "\nColor: " ).add( col.largeUserInfoString() );
	txt.add( "\nMarker size (pixels): " ).add( ps.disp_.pixsize_ );
	txt.add( "\nMarker type: " );
	txt.add( OD::MarkerStyle3D::toString(
		 (OD::MarkerStyle3D::Type)ps.disp_.markertype_) );
    }

    txt.add( "\n" ).add( getFileInfo() );
    setInfo( txt );
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    uiPickSetMgr mgr( this, Pick::Mgr() );
    MultiID curkey; if ( curioobj_ ) curkey = curioobj_->key();
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    mgr.mergeSets( curkey, &chsnnms );

    if ( !curkey.isEmpty() )
	selgrp_->fullUpdate( curkey );
}
