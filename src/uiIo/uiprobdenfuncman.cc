/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprobdenfuncman.cc,v 1.2 2010-02-05 12:08:49 cvsnanne Exp $";

#include "uiprobdenfuncman.h"

#include "uiioobjsel.h"
#include "uitextedit.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"

static const int cPrefWidth = 75;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("PDF file management",
				     "Manage probability density functions",
				     mTODOHelpID).nrstatusflds(1),
	           ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}


uiProbDenFuncMan::~uiProbDenFuncMan()
{
}


void uiProbDenFuncMan::mkFileInfo()
{
    if ( !curioobj_ ) { infofld->setText( "" ); return; }

    BufferString txt;
    txt += getFileInfo();

    mDynamicCastGet(ProbDenFuncTranslator*,tr,curioobj_->getTranslator())
    ProbDenFunc* pdf = tr ? tr->read( *curioobj_ ) : 0;
    if ( pdf )
	txt += "I can read the pdf!\n";

    infofld->setText( txt );
}
