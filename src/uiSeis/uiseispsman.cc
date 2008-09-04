/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiseispsman.cc,v 1.6 2008-09-04 13:31:45 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseispsman.h"
#include "uiprestkmergedlg.h"
#include "pixmap.h"
#include "seispsioprov.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiseismulticubeps.h"


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup("Pre-stack seismics management",
                                     "Manage Pre-stack seismics",
                                     "103.4.0").nrstatusflds(1),
	    	   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext() )
{
    createDefaultUI();
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    if ( !is2d )
    {
	manipgrp->addButton( ioPixmap("mergeseis.png"),
			     mCB(this,uiSeisPreStackMan,mergePush),
			     "Merge cubes" );
	manipgrp->addButton( ioPixmap("mkmulticubeps.png"),
			     mCB(this,uiSeisPreStackMan,mkMultiPush),
			     "Create Multi-Cube data store" );
    }

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


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiSeisMultiCubePS dlg( this );
    dlg.go();
    selgrp->fullUpdate( key );
}
