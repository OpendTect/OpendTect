/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisfileman.cc,v 1.107 2010-04-23 11:17:33 cvsbert Exp $";


#include "uiseisfileman.h"

#include "cbvsreadmgr.h"
#include "cubesampling.h"
#include "file.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "pixmap.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uimergeseis.h"
#include "uiseispsman.h"
#include "uiseisbrowser.h"
#include "uiseiscbvsimp.h"
#include "uiseisioobjinfo.h"
#include "uiseis2dfileman.h"
#include "uiseis2dgeom.h"
#include "uisplitter.h"
#include "uitaskrunner.h"

static const int cPrefWidth = 50;


uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic data",
                                     "103.1.0").nrstatusflds(1),
	    	   SeisTrcTranslatorGroup::ioContext())
    , is2d_(is2d)
{
    ctxt_.trglobexpr = is2d_ ? "2D" : "CBVS";
    createDefaultUI( true );
    selgrp->getListField()->doubleClicked.notify(
	    			is2d_ ? mCB(this,uiSeisFileMan,man2DPush)
				      : mCB(this,uiSeisFileMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    manipgrp->addButton( ioPixmap("copyobj.png"),
	    		 mCB(this,uiSeisFileMan,copyPush),
			 is2d ? "Copy lineset" : "Copy cube" );
    if ( is2d )
    {
	manipgrp->addButton( ioPixmap("man2d.png"),
			     mCB(this,uiSeisFileMan,man2DPush), "Manage lines");
	manipgrp->addButton( ioPixmap("dumpgeom.png"),
			     mCB(this,uiSeisFileMan,dump2DPush),
			     "Dump geometry" );
    }
    else
    {
	manipgrp->addButton( ioPixmap("mergeseis.png"),
			     mCB(this,uiSeisFileMan,mergePush),
			     "Merge blocks of inlines into cube" );
	manipgrp->addButton( ioPixmap("browseseis.png"),
			     mCB(this,uiSeisFileMan,browsePush),
			     "Browse/edit this cube" );
    }

    manipgrp->addButton( ioPixmap("man_ps.png"), mCB(this,uiSeisFileMan,manPS),
	    		 "Manage Pre-Stack data" );

    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );

    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}


const char* uiSeisFileMan::getDefKey() const
{
    const bool is2d = curioobj_ && SeisTrcTranslator::is2D( *curioobj_ );
    return is2d ? sKey::DefLineSet : sKey::DefCube;
}


void uiSeisFileMan::mkFileInfo()
{
    BufferString txt;
    SeisIOObjInfo oinf( curioobj_ );

    if ( oinf.isOK() )
    {

    if ( is2d_ )
    {
	BufferStringSet nms;
	oinf.getLineNames( nms );
	txt += "Number of lines: "; txt += nms.size();
	nms.erase(); oinf.getAttribNames( nms );
	if ( nms.size() > 1 )
	    { txt += "\nNumber of attributes: "; txt += nms.size(); }
    }

#define mRangeTxt(line) \
    txt += cs.hrg.start.line; txt += " - "; txt += cs.hrg.stop.line; \
    txt += " ["; txt += cs.hrg.step.line; txt += "]"

    const bool issidomain = ZDomain::isSIDomain( curioobj_->pars() );
    const bool zistm = (SI().zIsTime() && issidomain)
		    || (!SI().zIsTime() && !issidomain);

#define mAddZRangeTxt(memb) txt += zistm ? mNINT(1000*memb) : memb

    CubeSampling cs;
    if ( !is2d_ )
    {
	if ( oinf.getRanges(cs) )
	{
	    txt = "";
	    if ( !mIsUdf(cs.hrg.stop.inl) )
		{ txt += "Inline range: "; mRangeTxt(inl); }
	    if ( !mIsUdf(cs.hrg.stop.crl) )
		{ txt += "\nCrossline range: "; mRangeTxt(crl); }
	    txt += "\nZ-range: "; 
	    mAddZRangeTxt(cs.zrg.start); txt += " - ";
	    mAddZRangeTxt(cs.zrg.stop); 
	    txt += " ["; mAddZRangeTxt(cs.zrg.step); txt += "]";
	}
    }

    if ( curioobj_->pars().size() )
    {
	if ( curioobj_->pars().hasKey("Type") )
	{ txt += "\nType: "; txt += curioobj_->pars().find( "Type" ); }

	const char* optstr = "Optimized direction";
	if ( curioobj_->pars().hasKey(optstr) )
	{ txt += "\nOptimized direction: ";
	    txt += curioobj_->pars().find(optstr); }
	if ( curioobj_->pars().isTrue("Is Velocity") )
	{
	    txt += "\nVelocity Type: ";
	    const char* typstr = curioobj_->pars().find( "Velocity Type" );
	    txt += typstr ? typstr : "<unknown>";
	}
	if ( !issidomain )
	    { txt += "\nDomain: "; txt += zistm ? ZDomain::sKeyTWT()
						: ZDomain::sKeyDepth(); }
    }

    if ( !strcmp(curioobj_->translator(),"CBVS") )
    {
	CBVSSeisTrcTranslator* tri = CBVSSeisTrcTranslator::getInstance();
	if ( tri->initRead( new StreamConn(curioobj_->fullUserExpr(true),
				Conn::Read) ) )
	{
	    const BasicComponentInfo& bci = *tri->readMgr()->info().compinfo[0];
	    const DataCharacteristics::UserType ut = bci.datachar.userType();
	    BufferString etxt = eString(DataCharacteristics::UserType,ut);
	    txt += "\nStorage: "; txt += etxt.buf() + 4;
	}
	delete tri;
    }

    const int nrcomp = oinf.nrComponents();
    if ( nrcomp > 1 )
	{ txt += "\nNumber of components: "; txt += nrcomp; }


    } // if ( oinf.isOK() )

    if ( !txt.isEmpty() ) txt += "\n";
    txt += getFileInfo();

    infofld->setText( txt );
}


double uiSeisFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File::isEmpty(filenm) ) return -1;

    double totalsz = 0;
    nrfiles = 0;
    while ( true )
    {
	BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	if ( !File::exists(fullnm) ) break;
	
	totalsz += (double)File::getKbSize( fullnm );
	nrfiles++;
    }

    return totalsz;
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiMergeSeis dlg( this );
    dlg.go();
    selgrp->fullUpdate( key );
}


void uiSeisFileMan::dump2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    uiSeisDump2DGeom dlg( this, curioobj_ );
    dlg.go();
}


void uiSeisFileMan::browsePush( CallBacker* )
{
    if ( curioobj_ )
	uiSeisBrowser::doBrowse( this, *curioobj_, false );
}


void uiSeisFileMan::man2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiSeis2DFileMan dlg( this, *curioobj_ );
    dlg.go();

    selgrp->fullUpdate( key );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    if ( is2d_ )
    {
	uiSeisCopyLineSet dlg2d( this, curioobj_ );
	dlg2d.go();
    }
    else
    {
	mDynamicCastGet(const IOStream*,iostrm,curioobj_)
	if ( !iostrm ) { pErrMsg("IOObj not IOStream"); return; }

	uiSeisImpCBVS dlg( this, iostrm );
	dlg.go();
    }

    selgrp->fullUpdate( key );
}


void uiSeisFileMan::manPS( CallBacker* )
{
    uiSeisPreStackMan dlg( this, is2d_ );
    dlg.go();
}
