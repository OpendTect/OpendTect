/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksettools.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ioobjctxt.h"
#include "draw.h"
#include "picksetmanager.h"
#include "picksettr.h"
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
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( curioobj_->key(), uirv );
    if ( !ps )
    {
	txt.set( "Read error: '" ).add( uirv.getText() );
	txt.add( "'" ).add( "\n<No specific info available>\n" );
    }
    else
    {
	const bool ispoly = ps->isPolygon();
	const BufferString cat = ps->category();
	txt.add( "Type: " );
	if ( ispoly )
	    txt.add( "Polygon" );
	else if ( !cat.isEmpty() )
	    txt.add( cat );
	else
	    txt.add( "Pick Set" );

	MonitorLock ml( *ps );
	const int sz = ps->size();
	if ( sz < 1 )
	    txt.add( "Empty Pick Set." );
	else
	{
	    txt.add( " <" ).add( sz )
	       .add( ispoly ? " vertice" : " pick" );
	    if ( sz > 1 )
		txt.add( "s" );
	    if ( !ispoly && ps->first().hasDir() )
		txt.add( " (with directions)" );
	    if ( ispoly && sz > 2 )
	    {
		const float area = ps->getXYArea();
		if ( !mIsUdf(area) )
		    txt.add( ", area=" ).add( area );
	    }
	    txt.add( ">" );
	}

	const Pick::Set::Disp disp = ps->getDisp();
	Color col( disp.mkstyle_.color_ ); col.setTransparency( 0 );
	txt.add( "\nColor: " ).add( col.largeUserInfoString() );
	txt.add( "\nMarker size (pixels): " ).add( disp.mkstyle_.size_ );
	txt.add( "\nMarker type: " );
	txt.add(OD::MarkerStyle3D::TypeDef().getKey(disp.mkstyle_.type_));
    }

    txt.add( "\n" ).add( getFileInfo() );
    setInfo( txt );
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    DBKey curid;
    if ( curioobj_ )
	curid = curioobj_->key();

    uiMergePickSets dlg( this, curid );
    if ( dlg.go() )
	selgrp_->fullUpdate( curid );
}
