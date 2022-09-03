/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "picksettr.h"
#include "pickset.h"
#include "keystrs.h"
#include "survinfo.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)

static IOObjContext getIOObjContext( const char* fixedtrkey )
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    if ( fixedtrkey )
	ctxt.fixTranslator( fixedtrkey );

    BufferString types = sKey::PickSet();
    types.add( "`" ).add( sKey::Polygon() );
    ctxt.toselect_.require_.set( sKey::Type(), types.buf() );
    return ctxt;
}


uiPickSetMan::uiPickSetMan( uiParent* p, const char* fixedtrkey )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( toUiString("%1/%2")
                .arg( uiStrings::sPointSet(mPlural) )
                .arg( uiStrings::sPolygon(mPlural))),
	    mNoDlgTitle,
	    mODHelpKey(mPickSetManHelpID) )
	    .nrstatusflds(1).modal(false),
	    getIOObjContext(fixedtrkey))
{
    createDefaultUI();
    mergebut_ = addManipButton( "mergepicksets",
				uiStrings::phrMerge(uiStrings::sPointSet()),
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
	tt = uiStrings::phrMerge( mToUiStringTodo(chsnnms.getDispString(2)) );
    else
	tt = uiStrings::phrMerge(uiStrings::sPointSet());

    mergebut_->setToolTip( tt );
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,curioobj_,true, txt) )
    {
	BufferString msg( "Read error: '" ); msg += txt; msg += "'";
	txt = msg;
    }
    else
    {
	if ( !txt.isEmpty() )
	    ErrMsg( txt );

	const BufferString typ = curioobj_->pars().find( sKey::Type() );
	const bool ispoly = typ.isEqual( sKey::Polygon() );
	const bool havetype = !typ.isEmpty();
	if ( havetype )
	    txt.add( "Type: " ).add( typ );

	const int sz = ps->size();
	if ( sz < 1 )
	    txt.add( havetype ? " <empty>" : "Empty PointSet." );
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
	    if ( !ispoly && ps->get(0).hasDir() )
		txt += " (with directions)";

	    if ( ispoly && sz > 2 )
	    {
		const float area = ps->getXYArea();
		if ( !mIsUdf(area) )
		{
		    txt.add( ", area: " )
		       .add( getAreaString(area,SI().xyInFeet(),2,true) );
		}
	    }

	    if ( havetype )
		txt += ">";
	}

	OD::Color col( ps->disp_.color_ );
	col.setTransparency( 0 );
	txt.add( "\nColor: " ).add( col.largeUserInfoString() );
	txt.add( "\nMarker size (pixels): " ).add( ps->disp_.pixsize_ );
	txt.add( "\nMarker type: " ).add( MarkerStyle3D::getTypeString(
			sCast(MarkerStyle3D::Type,ps->disp_.markertype_)) );
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

    if ( !curkey.isUdf() )
	selgrp_->fullUpdate( curkey );
}
