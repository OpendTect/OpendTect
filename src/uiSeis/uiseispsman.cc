/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseispsman.cc,v 1.2 2007-10-24 07:52:44 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiseispsman.h"
#include "seispsioprov.h"
#include "uitextedit.h"
#include "uiioobjsel.h"


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Pre-stack seismics management",
                                     "Manage Pre-stack seismics",
                                     "103.4.0").nrstatusflds(1),
	    	   SeisPSTranslatorGroup::ioContext() )
{
    createDefaultUI();
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}


void uiSeisPreStackMan::mkFileInfo()
{
    BufferString txt;
    txt += getFileInfo();
    infofld->setText( txt );
}
