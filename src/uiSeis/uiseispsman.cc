/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseispsman.cc,v 1.21 2010-11-16 09:49:10 cvsbert Exp $";


#include "uiseispsman.h"

#include "keystrs.h"
#include "pixmap.h"
#include "posinfo.h"
#include "seispsioprov.h"

#include "uibutton.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiprestkmergedlg.h"
#include "uiseismulticubeps.h"
#include "uitextedit.h"


Notifier<uiSeisPreStackMan>* uiSeisPreStackMan::fieldsCreated()
{
    static Notifier<uiSeisPreStackMan> FieldsCreated(0);
    return &FieldsCreated;
}


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup("Pre-stack seismics management",
				 "Manage Pre-stack seismics",
				 is2d ? "103.4.1" : "103.4.0").nrstatusflds(1),
	    	   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
{
    createDefaultUI( true );
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    if ( !is2d )
    {
	manipgrp->addButton( "copyobj.png",
			     mCB(this,uiSeisPreStackMan,copyPush),
			     "Copy data store" );
	manipgrp->addButton( "mergeseis.png",
			     mCB(this,uiSeisPreStackMan,mergePush),
			     "Merge data stores" );
	manipgrp->addButton( "mkmulticubeps.png",
			     mCB(this,uiSeisPreStackMan,mkMultiPush),
			     "Create Multi-Cube data store" );
    }

    fieldsCreated()->trigger( this );
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}


void uiSeisPreStackMan::mkFileInfo()
{
    BufferString txt;
    if ( is2d_ )
    {
	BufferStringSet nms;
	SPSIOPF().getLineNames( *curioobj_, nms );
	if ( nms.size() > 0 )
	{
	    if ( nms.size() > 4 )
		{ txt = "Number of lines: "; txt += nms.size(); }
	    else
	    {
		if ( nms.size() == 1 )
		    { txt = "Line: "; txt += nms.get( 0 ); }
		else
		{
		    txt = "Lines: ";
		    for ( int idx=0; idx<nms.size(); idx++ )
		    {
			if ( idx ) txt += ", ";
			txt += nms.get( idx );
		    }
		}
	    }
	    txt += "\n\n";
	}
    }
    else
    {
	SeisPS3DReader* rdr = SPSIOPF().get3DReader( *curioobj_ );
	const PosInfo::CubeData& cd = rdr->posData();
	txt = "Total number of gathers: "; txt += cd.totalSize(); txt += "\n";
	const bool haveinlstep = cd.haveInlStepInfo();
	const bool havecrlstep = cd.haveCrlStepInfo();
	const bool havebothsteps = haveinlstep && havecrlstep;
	StepInterval<int> rg; cd.getInlRange( rg );
	txt += "Inline range: ";
	txt += rg.start; txt += " - "; txt += rg.stop;
	if ( cd.haveInlStepInfo() )
	{ txt += " step "; txt += rg.step; }
	txt += "\n";
	cd.getCrlRange( rg );
	txt += "Crossline range: ";
	txt += rg.start; txt += " - "; txt += rg.stop;
	if ( cd.haveCrlStepInfo() )
	{ txt += " step "; txt += rg.step; }
	txt += "\n\n";
	delete rdr;
    }

    txt += getFileInfo();
    setInfo( txt );
}


void uiSeisPreStackMan::copyPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackCopyDlg dlg( this, key );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisPreStackMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackMergeDlg dlg( this );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    const MultiID key( curioobj_ ? curioobj_->key() : MultiID("") );
    uiSeisMultiCubePS dlg( this );
    dlg.go();
    selgrp_->fullUpdate( key );
}
