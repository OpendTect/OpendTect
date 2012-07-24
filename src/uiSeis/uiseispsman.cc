/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiseispsman.cc,v 1.30 2012-07-24 09:03:15 cvskris Exp $";


#include "uiseispsman.h"

#include "keystrs.h"
#include "pixmap.h"
#include "posinfo.h"
#include "zdomain.h"
#include "seispsioprov.h"
#include "seisioobjinfo.h"

#include "uibutton.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiprestkmergedlg.h"
#include "uiseismulticubeps.h"
#include "uitextedit.h"

mDefineInstanceCreatedNotifierAccess(uiSeisPreStackMan)


#define mCapt \
    is2d ? "Manage 2D Pre-stack seismics" : "Manage 3D Pre-stack seismics"
#define mHelpID is2d ? "103.4.1" : "103.4.0"
uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(mCapt,mNoDlgTitle,mHelpID)
		     .nrstatusflds(1),
	    	   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
{
    createDefaultUI( true );
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    if ( !is2d )
    {
	manipgrp->addButton( "copyobj", "Copy data store",
			     mCB(this,uiSeisPreStackMan,copyPush) );
	manipgrp->addButton( "mergeseis", "Merge data stores",
			     mCB(this,uiSeisPreStackMan,mergePush) );
	manipgrp->addButton( "mkmulticubeps","Create Multi-Cube data store",
			     mCB(this,uiSeisPreStackMan,mkMultiPush) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}


void uiSeisPreStackMan::mkFileInfo()
{
    BufferString txt;
    SeisIOObjInfo objinf( curioobj_ );
    if ( objinf.isOK() )
    {
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
	    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *curioobj_ );
	    const PosInfo::CubeData& cd = rdr->posData();
	    txt.add( "Total number of gathers: " ).add( cd.totalSize() );
	    StepInterval<int> rg; cd.getInlRange( rg );
	    txt.add( "\nInline range: " )
			.add( rg.start ).add( " - " ).add( rg.stop );
	    if ( cd.haveInlStepInfo() )
		{ txt.add( " step " ).add( rg.step ); }
	    cd.getCrlRange( rg );
	    txt.add( "\nCrossline range: " )
			.add( rg.start ).add( " - " ).add( rg.stop );
	    if ( cd.haveCrlStepInfo() )
		{ txt.add( " step " ).add( rg.step ); }
	}
	txt.add("\n");
	CubeSampling cs;
	if ( objinf.getRanges(cs) )
	{
	    const bool zistm = objinf.isTime();
	    const ZDomain::Def& zddef = objinf.zDomainDef();
#	    define mAddZValTxt(memb) .add(zistm ? mNINT32(1000*memb) : memb)
	    txt.add(zddef.userName()).add(" range ")
		.add(zddef.unitStr(true)).add(": ") mAddZValTxt(cs.zrg.start)
		.add(" - ") mAddZValTxt(cs.zrg.stop) 
		.add(" [") mAddZValTxt(cs.zrg.step) .add("]\n");
	}
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
