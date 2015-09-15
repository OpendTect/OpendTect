/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseispsman.h"

#include "keystrs.h"
#include "posinfo.h"
#include "zdomain.h"
#include "seispsioprov.h"
#include "seisioobjinfo.h"

#include "uibutton.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uipixmap.h"
#include "uiprestkmergedlg.h"
#include "uiprestkcopy.h"
#include "uiseismulticubeps.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiSeisPreStackMan)

#define mHelpID is2d ? mODHelpKey(mSeisPrestackMan2DHelpID) : \
                       mODHelpKey(mSeisPrestackMan3DHelpID)
uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(createCaption(is2d),mNoDlgTitle,mHelpID)
		     .nrstatusflds(1).modal(false),
		   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
    , copybut_(0)
    , mergebut_(0)
{
    createDefaultUI( true );
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    if ( !is2d )
    {
	copybut_ = manipgrp->addButton( "copyobj", "Copy data store",
					mCB(this,uiSeisPreStackMan,copyPush) );
	mergebut_ = manipgrp->addButton( "mergeseis", "Merge data stores",
					mCB(this,uiSeisPreStackMan,mergePush) );
	manipgrp->addButton( "mkmulticubeps",
			     "Create/Edit Multi-Cube data store",
			     mCB(this,uiSeisPreStackMan,mkMultiPush) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}



uiString uiSeisPreStackMan::createCaption( bool is2d )
{
    return is2d
	? tr("Manage 2D Prestack Seismics")
	: tr("Manage 3D Prestack Seismics");
}


#define mSetButToolTip(but,str1,cursel,str2,deftt) \
    if ( !but->sensitive() ) \
	but->setToolTip( deftt ); \
    else \
    { \
	tt.setEmpty(); \
	tt.add( str1 ).add( cursel ).add( str2 ); \
	but->setToolTip( tr(tt) ); \
    }

void uiSeisPreStackMan::ownSelChg()
{
    if ( is2d_ )
	return;

    BufferString tt,cursel;
    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    copybut_->setSensitive( curioobj_ );
    mergebut_->setSensitive( curioobj_ );
    mSetButToolTip(copybut_,"Make a copy of '",cursel,"'","Copy data store");
    BufferStringSet selnms;
    selgrp_->getChosen( selnms );
    if ( selnms.size() > 1 )
    {
	mSetButToolTip(mergebut_,"Merge ",selnms.getDispString(2),"",
		       "Merge data store");
    }
    else
	mergebut_->setToolTip( "Merge data store" );
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
	    txt.set( "Line" ).add( nms.size() != 1 ? "s: " : ": " )
		.add( nms.getDispString(3,false) );
	}
	else
	{
	    PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( *curioobj_ );
	    if ( rdr )
	    {
		const PosInfo::CubeData& cd = rdr->posData();
		txt.add( "Total number of gathers: " ).add( cd.totalSize() );
		StepInterval<int> rg; cd.getInlRange( rg );
		txt.add( "\nInline range: " )
			    .add( rg.start ).add( " - " ).add( rg.stop );
		if ( cd.haveInlStepInfo() )
		    { txt.add( " [" ).add( rg.step ).add( "]" ); }
		cd.getCrlRange( rg );
		txt.add( "\nCrossline range: " )
			    .add( rg.start ).add( " - " ).add( rg.stop );
		if ( cd.haveCrlStepInfo() )
		    { txt.add( " [" ).add( rg.step ).add( "]" ); }
	    }
	}
	txt.add( "\n" );

	TrcKeyZSampling cs;
	if ( objinf.getRanges(cs) )
	{
	    const bool zistm = objinf.isTime();
	    const ZDomain::Def& zddef = objinf.zDomainDef();
#	    define mAddZValTxt(memb) .add(zistm ? mNINT32(1000*memb) : memb)
	    txt.add(zddef.userName().getFullString()).add(" range ")
		.add(zddef.unitStr(true)).add(": ") mAddZValTxt(cs.zsamp_.start)
		.add(" - ") mAddZValTxt(cs.zsamp_.stop)
		.add(" [") mAddZValTxt(cs.zsamp_.step) .add("]\n");
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
    BufferStringSet selnms;
    selgrp_->getChosen( selnms );
    dlg.setInputIds( selnms );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    MultiID key; const char* toedit = 0;
    if ( curioobj_ )
    {
	key = curioobj_->key();
	if ( curioobj_->translator() == "MultiCube" )
	    toedit = key.buf();
    }
    uiSeisMultiCubePS dlg( this, toedit );
    dlg.go();
    selgrp_->fullUpdate( key );
}
