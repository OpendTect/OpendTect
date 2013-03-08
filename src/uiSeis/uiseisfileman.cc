/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseisfileman.h"

#include "cbvsreadmgr.h"
#include "cubesampling.h"
#include "file.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "segydirecttr.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "zdomain.h"
#include "separstr.h"
#include "timedepthconv.h"

#include "ui2dgeomman.h"
#include "uitoolbutton.h"
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

mDefineInstanceCreatedNotifierAccess(uiSeisFileMan)

#define mCapt is2d ? "Manage 2D Seismics" : "Manage 3D Seismics"
#define mHelpID is2d ? "103.1.11" : "103.1.0"
uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(mCapt,mNoDlgTitle,mHelpID).nrstatusflds(1),
	    	   SeisTrcTranslatorGroup::ioContext())
    , is2d_(is2d)
    , browsebut_(0)
{
    if ( is2d_ )
	ctxt_.toselect.allowtransls_ = "2D";
    else
    {
	FileMultiString fms( CBVSSeisTrcTranslator::translKey() );
	fms += SEGYDirectSeisTrcTranslator::translKey();
	ctxt_.toselect.allowtransls_ = fms;
    }
    createDefaultUI( true );
    selgrp_->getListField()->doubleClicked.notify(
	    			is2d_ ? mCB(this,uiSeisFileMan,man2DPush)
				      : mCB(this,uiSeisFileMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    manipgrp->addButton( "copyobj", is2d ? "Copy lineset" : "Copy cube",
			 mCB(this,uiSeisFileMan,copyPush) );
    if ( is2d )
    {
	manipgrp->addButton( "man2d", "Manage lines",
				mCB(this,uiSeisFileMan,man2DPush) );
	manipgrp->addButton( "dumpgeom", "Dump geometry",
				mCB(this,uiSeisFileMan,dump2DPush) );
	manipgrp->addButton( "man2dgeom.png", "Manage 2D geometry",
				mCB(this,uiSeisFileMan,man2DGeom) );
    }
    else
    {
	manipgrp->addButton( "mergeseis", "Merge cube parts into one cube",
				mCB(this,uiSeisFileMan,mergePush) );
	browsebut_ = manipgrp->addButton( "browseseis",
				"Browse/edit this cube",
				mCB(this,uiSeisFileMan,browsePush) );
    }

    manipgrp->addButton( "man_ps", "Manage Pre-Stack data",
			 mCB(this,uiSeisFileMan,manPS) );

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}


void uiSeisFileMan::ownSelChg()
{
    if ( !browsebut_ || !curioobj_ ) return;
    
    browsebut_->setSensitive( curimplexists_ && strcmp(curioobj_->translator(),
		SEGYDirectSeisTrcTranslator::translKey() ) );
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

#define mAddRangeTxt(line) \
    .add(" range: ").add(cs.hrg.start.line).add(" - ").add(cs.hrg.stop.line) \
    .add(" [").add(cs.hrg.step.line).add("]")
#define mAddZValTxt(memb) .add(zistm ? mNINT32(1000*memb) : memb)

    const bool zistm = oinf.isTime();
    const ZDomain::Def& zddef = oinf.zDomainDef();
    CubeSampling cs;
    if ( !is2d_ )
    {
	if ( oinf.getRanges(cs) )
	{
	    txt = "";
	    if ( !mIsUdf(cs.hrg.stop.inl) )
		{ txt.add("Inline") mAddRangeTxt(inl); }
	    if ( !mIsUdf(cs.hrg.stop.crl) )
		{ txt.add("\nCrossline") mAddRangeTxt(crl); }
	    float area = SI().computeArea( cs.hrg.inlRange(),
		    			   cs.hrg.crlRange() );
	    txt.add("\nArea: ").add( getAreaString( area, true, 0 ) );

	    txt.add("\n").add(zddef.userName()).add(" range ")
		.add(zddef.unitStr(true)).add(": ") mAddZValTxt(cs.zrg.start)
		.add(" - ") mAddZValTxt(cs.zrg.stop) 
		.add(" [") mAddZValTxt(cs.zrg.step) .add("]");
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
	    Interval<float> topvavg, botvavg;
	    txt += "\nVelocity Type: ";
	    const char* typstr = curioobj_->pars().find( "Velocity Type" );
	    txt += typstr ? typstr : "<unknown>";

	    if ( curioobj_->pars().get(VelocityStretcher::sKeyTopVavg(),topvavg)
	    && curioobj_->pars().get(VelocityStretcher::sKeyBotVavg(),botvavg) )
	    {
		StepInterval<float> rg;
		StepInterval<float> zrg = SI().zRange(true);

		if ( SI().zIsTime() )
		{
		    rg.start = zrg.start * topvavg.start / 2;
		    rg.stop = zrg.stop * botvavg.stop / 2;
		    rg.step = (rg.stop-rg.start) / zrg.nrSteps();
		    txt += "\nDepth Range ";
		    txt += ZDomain::Depth().unitStr(true);
		}

		else 
		{
		    rg.start = 2 * zrg.start / topvavg.stop;
		    rg.stop = 2 * zrg.stop / botvavg.start;
		    rg.step = (rg.stop-rg.start) / zrg.nrSteps();
		    rg.scale( mCast( float, ZDomain::Time().userFactor() ) );
		    txt += "\nTime Range ";
		    txt += ZDomain::Time().unitStr(true);  
		}

		txt += ": ";
		txt += rg.start;
		txt += " - ";
		txt += rg.stop;
	    }
	}
    }

    if ( !strcmp(curioobj_->translator(),CBVSSeisTrcTranslator::translKey()) )
    {
	CBVSSeisTrcTranslator* tri = CBVSSeisTrcTranslator::getInstance();
	if ( tri->initRead( new StreamConn(curioobj_->fullUserExpr(true),
				Conn::Read) ) )
	{
	    const BasicComponentInfo& bci = *tri->readMgr()->info().compinfo_[0];
	    const DataCharacteristics::UserType ut = bci.datachar.userType();
	    BufferString etxt = DataCharacteristics::getUserTypeString(ut);
	    txt += "\nStorage: "; txt += etxt.buf() + 4;
	}
	delete tri;
    }

    const int nrcomp = oinf.nrComponents();
    if ( nrcomp > 1 )
	{ txt += "\nNumber of components: "; txt += nrcomp; }


    } // if ( oinf.isOK() )

    if ( txt.isEmpty() ) txt = "<Empty>";
    txt.add( "\n" ).add( getFileInfo() );

    setInfo( txt );
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
    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::dump2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    uiSeisDump2DGeom dlg( this, curioobj_ );
    dlg.go();
}


void uiSeisFileMan::man2DGeom( CallBacker* )
{
    ui2DGeomManageDlg dlg( this );
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

    selgrp_->fullUpdate( key );
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

    selgrp_->fullUpdate( key );
}


void uiSeisFileMan::manPS( CallBacker* )
{
    uiSeisPreStackMan dlg( this, is2d_ );
    dlg.go();
}
