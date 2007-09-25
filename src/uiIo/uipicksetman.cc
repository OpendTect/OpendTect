/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
 RCS:           $Id: uipicksetman.cc,v 1.7 2007-09-25 10:28:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"

#include "ctxtioobj.h"
#include "picksettr.h"
#include "pickset.h"
#include "draw.h"

static const int cPrefWidth = 75;

uiPickSetMan::uiPickSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("PickSet file management",
				     "Manage picksets",
				     "107.1.0").nrstatusflds(1),
	           PickSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp->getManipGroup()->addButton( "mergepicksets.png",
	    		mCB(this,uiPickSetMan,mergeSets), "Merge pick sets" );
    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { infofld->setText( "" ); return; }

    BufferString txt;
    Pick::Set ps;
    if ( !PickSetTranslator::retrieve(ps,curioobj_,txt) )
    {
	BufferString msg( "Read error: '" ); msg += txt; msg += "'";
	txt = msg;
    }
    else
    {
	if ( !txt.isEmpty() )
	    ErrMsg( txt );

	if ( ps.isEmpty() )
	    txt = "Empty Pick Set\n";
	else
	{
	    txt = "Number of picks: ";
	    txt += ps.size(); txt += "\n";
	}
	if ( ps[0].hasDir() )
	    txt += "Pick Set with directions\n";

	Color col( ps.disp_.color_ ); col.setTransparency( 0 );
	char buf[20]; col.fill( buf ); replaceCharacter( buf, '`', '-' );
	txt += "Color (R-G-B): "; txt += buf;
	txt += "\nMarker size (pixels): "; txt += ps.disp_.pixsize_;
	txt += "\nMarker type: ";
	txt += eString(MarkerStyle3D::Type,ps.disp_.markertype_);
    }

    txt += "\n";
    txt += getFileInfo();
    infofld->setText( txt );
}


class uiPickSetManPickSetMgr : public uiPickSetMgr
{
public:
uiPickSetManPickSetMgr( uiParent* p ) : uiPickSetMgr(Pick::Mgr()), par_(p) {}
uiParent* parent() { return par_; }

    uiParent*	par_;

};

void uiPickSetMan::mergeSets( CallBacker* )
{
    uiPickSetManPickSetMgr mgr( this );
    MultiID curkey; if ( curioobj_ ) curkey = curioobj_->key();
    mgr.mergeSets( curkey );

    if ( curkey != "" )
	selgrp->fullUpdate( curkey );
}
