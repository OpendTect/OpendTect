/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseisfileman.cc,v 1.64 2007-08-29 11:45:46 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseisfileman.h"
#include "uiseispsman.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "cbvsreadmgr.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "uiseisioobjinfo.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uimergeseis.h"
#include "uiseiscbvsimp.h"
#include "uiseis2dgeom.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "pixmap.h"
#include "survinfo.h"
#include "oddirs.h"
#include "filegen.h"
#include "filepath.h"
#include "keystrs.h"


static const int cPrefWidth = 50;


uiSeisFileMan::uiSeisFileMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic cubes",
                                     "103.1.0").nrstatusflds(1),
	    	   SeisTrcTranslatorGroup::ioContext() )
{
    ctxt_.trglobexpr = "CBVS`2D";
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    cpym2dbut = manipgrp->addButton( ioPixmap("copyobj.png"),
	    			     mCB(this,uiSeisFileMan,copyMan2DPush),
	    			     "Copy cube" );
    manipgrp->setAlternative( cpym2dbut, ioPixmap("man2d.png"), "Manage lines");

    mrgdmpbut = manipgrp->addButton( ioPixmap("mergeseis.png"),
	    			     mCB(this,uiSeisFileMan,mergeDump2DPush),
	    			     "Merge blocks of inlines into cube" );
    manipgrp->setAlternative( mrgdmpbut, ioPixmap("dumpgeom.png"),
			      "Dump geometry" );

    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );

    IOObjContext psctxt( SeisPSTranslatorGroup::ioContext() );
    IOObj* psioobj = IOM().getFirst( psctxt );
    if ( psioobj )
    {
	delete psioobj;
	uiToolButton* psbut = new uiToolButton( this, "&Pre-Stack",
				    ioPixmap("man_ps.png"),
				    mCB(this,uiSeisFileMan,manPS) );
	psbut->setToolTip( "Manage Pre-Stack data" );
	psbut->attach( rightBorder );
    }

    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}


void uiSeisFileMan::ownSelChg()
{
    if ( !curioobj_ ) return;

    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    const bool is2d = curioobj_ && SeisTrcTranslator::is2D( *curioobj_ );
    manipgrp->useAlternative( cpym2dbut, is2d );
    manipgrp->useAlternative( mrgdmpbut, is2d );
    cpym2dbut->setSensitive( is2d
	    		|| (curioobj_ && curioobj_->implExists(true)) );
}


void uiSeisFileMan::mkFileInfo()
{
    BufferString txt;
    const bool is2d = SeisTrcTranslator::is2D( *curioobj_ );
    if ( is2d )
    {
	BufferString fnm( curioobj_->fullUserExpr(true) );
	Seis2DLineSet lset( fnm );
	txt += "Number of lines: "; txt += lset.nrLines();
    }


#define mRangeTxt(line) \
    txt += cs.hrg.start.line; txt += " - "; txt += cs.hrg.stop.line; \
    txt += " ["; txt += cs.hrg.step.line; txt += "]"

#define mZRangeTxt(memb) \
    txt += SI().zIsTime() ? mNINT(1000*memb) : memb

    CubeSampling cs;
    if ( !is2d )
    {
	SeisIOObjInfo oinf( *curioobj_ );
	if ( oinf.getRanges(cs) )
	{
	    txt = "";
	    if ( !mIsUdf(cs.hrg.stop.inl) )
		{ txt += "Inline range: "; mRangeTxt(inl); }
	    if ( !mIsUdf(cs.hrg.stop.crl) )
		{ txt += "\nCrossline range: "; mRangeTxt(crl); }
	    txt += "\nZ-range: "; 
	    mZRangeTxt(cs.zrg.start); txt += " - "; mZRangeTxt(cs.zrg.stop); 
	    txt += " ["; mZRangeTxt(cs.zrg.step); txt += "]";
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

    if ( !txt.isEmpty() ) txt += "\n";
    txt += getFileInfo();

    infofld->setText( txt );
}


double uiSeisFileMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File_isEmpty(filenm) ) return -1;

    double totalsz = 0;
    nrfiles = 0;
    while ( true )
    {
	BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	if ( !File_exists(fullnm) ) break;
	
	totalsz += (double)File_getKbSize( fullnm );
	nrfiles++;
    }

    return totalsz;
}


void uiSeisFileMan::mergeDump2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;
    const bool is2d = SeisTrcTranslator::is2D( *curioobj_ );
    if ( is2d )
    {
	uiSeisDump2DGeom dlg( this, curioobj_ );
	dlg.go();
    }
    else
    {
	const MultiID key( curioobj_->key() );
	uiMergeSeis dlg( this );
	dlg.go();
	selgrp->fullUpdate( key );
    }
}



class uiSeis2DMan : public uiDialog
{
public:

uiSeis2DMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup("Seismic file management",
				 "Manage 2D seismic lines",
				 "103.1.3"))
{
    setCtrlStyle( LeaveOnly );

    objinfo = new uiSeisIOObjInfo( ioobj );
    lineset = new Seis2DLineSet( ioobj.fullUserExpr(true) );

    uiGroup* topgrp = new uiGroup( this, "" );
    linelist = new uiLabeledListBox( topgrp, "2D lines", true,
	    			     uiLabeledListBox::AboveMid );
    linelist->box()->selectionChanged.notify( mCB(this,uiSeis2DMan,lineSel) );

    uiManipButGrp* linebutgrp = new uiManipButGrp( linelist );
    linebutgrp->addButton( uiManipButGrp::Rename, 
	    		   mCB(this,uiSeis2DMan,renameLine), "Rename line" );
    linebutgrp->attach( rightTo, linelist->box() );

    attriblist = new uiLabeledListBox( topgrp, "Attributes", true,
				       uiLabeledListBox::AboveMid );
    attriblist->box()->selectionChanged.notify( 
	    				mCB(this,uiSeis2DMan,attribSel) );
    attriblist->attach( rightTo, linelist );

    uiManipButGrp* butgrp = new uiManipButGrp( attriblist );
    butgrp->addButton( uiManipButGrp::Rename,
	    	       mCB(this,uiSeis2DMan,renameAttrib), "Rename attribute" );
    butgrp->addButton( uiManipButGrp::Remove,
	    	       mCB(this,uiSeis2DMan,removeAttrib),
		       "Remove selected attribute(s)" );
    butgrp->attach( rightTo, attriblist->box() );

    infofld = new uiTextEdit( this, "File Info", true );
    infofld->attach( alignedBelow, topgrp );
    infofld->attach( widthSameAs, topgrp );
    infofld->setPrefHeightInChar( 5 );
    infofld->setPrefWidthInChar( 50 );
    
    fillLineBox();
    lineSel(0);
}


~uiSeis2DMan()
{
    delete objinfo;
    delete lineset;
}


void fillLineBox()
{
    uiListBox* lb = linelist->box();
    const int curitm = lb->size() ? lb->currentItem() : 0;
    BufferStringSet linenames;
    objinfo->getLineNames( linenames );
    lb->empty();
    lb->addItems( linenames );
    lb->setSelected( curitm );
}


void lineSel( CallBacker* )
{
    BufferStringSet sellines;
    linelist->box()->getSelectedItems( sellines );
    BufferStringSet sharedattribs;
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	BufferStringSet attrs;
	objinfo->getAttribNamesForLine( sellines.get(idx), attrs );
	if ( !idx )
	{
	    sharedattribs = attrs;
	    continue;
	}

	BufferStringSet strs2rem;
	for ( int ida=0; ida<sharedattribs.size(); ida++ )
	{
	    const char* str = sharedattribs.get(ida);
	    int index = attrs.indexOf( str );
	    if ( index<0 ) strs2rem.add( str );
	}

	for ( int ida=0; ida<strs2rem.size(); ida++ )
	{
	    const int index = sharedattribs.indexOf( strs2rem.get(ida) );
	    sharedattribs.remove( index );
	}
    }

    attriblist->box()->empty();
    sharedattribs.sort();
    attriblist->box()->addItems( sharedattribs );
    attriblist->box()->setSelected( 0, true );
}


void attribSel( CallBacker* )
{
    infofld->setText( "" );
    BufferStringSet linenms, attribnms;
    linelist->box()->getSelectedItems( linenms );
    attriblist->box()->getSelectedItems( attribnms );
    if ( linenms.isEmpty() || attribnms.isEmpty() )
    { infofld->setText(""); return; }

    const LineKey linekey( linenms.get(0), attribnms.get(0) );
    const int lineidx = lineset->indexOf( linekey );
    if ( lineidx < 0 ) return;


    BufferString txt;
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    lineset->getRanges( lineidx, trcrg, zrg );
    txt += "Trace range: "; txt += trcrg.start; txt += " - "; txt += trcrg.stop;
    txt += " ["; txt += trcrg.step; txt += "]";
    txt += "\nZ-range: "; mZRangeTxt(zrg.start); txt += " - ";
    mZRangeTxt(zrg.stop); txt += " ["; mZRangeTxt(zrg.step); txt += "]";

    const IOPar& iopar = lineset->getInfo( lineidx );
    BufferString fname = iopar.find( sKey::FileName );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    fname = fp.fullPath();

    txt += "\nLocation: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; 
    txt += uiObjFileMan::getFileSizeString( File_getKbSize(fname) );
    infofld->setText( txt );
}


void removeAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attriblist->box()->getSelectedItems( attribnms );
    if ( attribnms.isEmpty()
      || !uiMSG().askGoOn("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    BufferStringSet sellines;
    linelist->box()->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linename = sellines.get(idx);
	for ( int ida=0; ida<attribnms.size(); ida++ )
	{
	    LineKey linekey( linename, attribnms.get(ida) );
	    if ( !lineset->remove(linekey) )
		uiMSG().error( "Could not remove attribute" );
	}
    }

    fillLineBox();
}


bool rename( const char* oldnm, BufferString& newnm )
{
    BufferString titl( "Rename '" );
    titl += oldnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(oldnm) );
    if ( !dlg.go() ) return false;
    newnm = dlg.text();
    return newnm != oldnm;
}


void renameLine( CallBacker* )
{
    BufferStringSet linenms;
    linelist->box()->getSelectedItems( linenms );
    if ( linenms.isEmpty() ) return;

    const char* linenm = linenms.get(0);
    BufferString newnm;
    if ( !rename(linenm,newnm) ) return;
    
    if ( linelist->box()->isPresent(newnm) )
    {
	uiMSG().error( "Linename already in use" );
	return;
    }

    if ( !lineset->renameLine( linenm, newnm ) )
    {
	uiMSG().error( "Could not rename line" );
	return;
    }

    fillLineBox();
}


void renameAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attriblist->box()->getSelectedItems( attribnms );
    if ( attribnms.isEmpty() ) return;

    const char* attribnm = attribnms.get(0);
    BufferString newnm;
    if ( !rename(attribnm,newnm) ) return;

    if ( attriblist->box()->isPresent(newnm) )
    {
	uiMSG().error( "Attribute name already in use" );
	return;
    }

    BufferStringSet sellines;
    linelist->box()->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linenm = sellines.get(idx);
	LineKey oldlk( linenm, attribnm );
	if ( !lineset->rename(oldlk,LineKey(linenm,newnm)) )
	{
	    BufferString err( "Could not rename attribute: " );
	    err += oldlk;
	    uiMSG().error( err );
	    continue;
	}
    }

    lineSel(0);
}

protected:

    uiLabeledListBox*	linelist;
    uiLabeledListBox*	attriblist;
    uiTextEdit*		infofld;

    Seis2DLineSet*	lineset;
    uiSeisIOObjInfo*	objinfo;

};


void uiSeisFileMan::copyMan2DPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool is2d = SeisTrcTranslator::is2D( *curioobj_ );
    const MultiID key( curioobj_->key() );

    if ( is2d )
    {
	uiSeis2DMan dlg( this, *curioobj_ );
	dlg.go();
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
    uiSeisPreStackMan dlg( this );
    dlg.go();
}
