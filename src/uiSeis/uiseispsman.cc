/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiseispsman.cc,v 1.4 2007-12-18 16:46:56 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseispsman.h"
#include "uiprestkmergedlg.h"
#include "pixmap.h"
#include "seispsioprov.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
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
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    mrgbut = manipgrp->addButton( ioPixmap("mergeseis.png"),
	    			mCB(this,uiSeisPreStackMan,mergePush),
				"Merge cubes" );
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


void uiSeisPreStackMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackMergeDlg dlg( this );
    dlg.go();
    selgrp->fullUpdate( key );
}
