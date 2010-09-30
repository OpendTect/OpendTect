/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipicksetman.cc,v 1.12 2010-09-30 10:03:34 cvsnageswara Exp $";

#include "uipicksetman.h"
#include "uipicksetmgr.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "picksettr.h"
#include "pickset.h"

static const int cPrefWidth = 75;

Notifier<uiPickSetMan>* uiPickSetMan::fieldsCreated()
{
    static Notifier<uiPickSetMan> FieldsCreated(0);
    return &FieldsCreated;
}


uiPickSetMan::uiPickSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("PickSet file management",
				     "Manage picksets",
				     "105.0.6").nrstatusflds(1),
	           PickSetTranslatorGroup::ioContext())
    , lastexternal_(0)
{
    createDefaultUI();
    selgrp->getManipGroup()->addButton( "mergepicksets.png",
	    		mCB(this,uiPickSetMan,mergeSets), "Merge pick sets" );
    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
    fieldsCreated()->trigger( this );
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::addTool( uiButton* but )
{
    if ( lastexternal_ )
	but->attach( rightOf, lastexternal_ );
    else
    {
	but->attach( ensureBelow, selgrp );
	infofld->attach( ensureBelow, but );
    }

    lastexternal_ = but;
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { infofld->setText( "" ); return; }

    BufferString txt;
    Pick::Set ps;
    if ( !PickSetTranslator::retrieve(ps,curioobj_,true, txt) )
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
