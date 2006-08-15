/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2003
 RCS:           $Id: uiattrsetman.cc,v 1.2 2006-08-15 11:37:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrsetman.h"

#include "uiioobjsel.h"
#include "uitextedit.h"

#include "attribdescsettr.h"
#include "ctxtioobj.h"

static const int cPrefWidth = 75;

uiAttrSetMan::uiAttrSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Attribute Set file management",
				     "Manage attribute sets",
				     "107.1.0").nrstatusflds(1),
	           AttribDescSetTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );

    selChg( this );
}


uiAttrSetMan::~uiAttrSetMan()
{
}


void uiAttrSetMan::mkFileInfo()
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
