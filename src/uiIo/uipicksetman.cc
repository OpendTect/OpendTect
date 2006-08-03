/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
 RCS:           $Id: uipicksetman.cc,v 1.1 2006-08-03 18:56:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uipicksetman.h"

#include "uitextedit.h"

#include "ctxtioobj.h"
#include "picksettr.h"


uiPickSetMan::uiPickSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("PickSet file management",
				     "Manage picksets",
				     "107.1.0").nrstatusflds(1),
	           PickSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::mkFileInfo()
{
    if ( !curioobj_ )
    {
	infofld->setText( "" );
	return;
    }

    BufferString txt;
    txt += getFileInfo();

    infofld->setText( txt );
}
