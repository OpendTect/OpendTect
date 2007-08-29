/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseispsman.cc,v 1.1 2007-08-29 09:52:40 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseispsman.h"
#include "seispsioprov.h"
#include "uitextedit.h"
#include "uiioobjsel.h"


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Pre-stack seismics management",
                                     "Manage Pre-stack seismics",
                                     "103.4.0"),
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
