/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseisfileman.cc,v 1.48 2004-11-02 13:56:44 bert Exp $
________________________________________________________________________

-*/


#include "uiseisfileman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "cbvsio.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "uiseisioobjinfo.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uimergeseis.h"
#include "uiseiscbvsimp.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "pixmap.h"
#include "survinfo.h"
#include "filegen.h"
#include "filepath.h"
#include "keystrs.h"


static const int cPrefWidth = 50;


uiSeisFileMan::uiSeisFileMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic cubes",
                                     "103.1.0").nrstatusflds(1),
	    	   *mMkCtxtIOObj(SeisTrc) )
{
    ctio.ctxt.trglobexpr = "CBVS`2D";
    createDefaultUI( "cbvs" );

    const ioPixmap copypm( GetDataFileName("copyobj.png") );
    copybut = manipgrp->addButton( copypm, mCB(this,uiSeisFileMan,copyPush),
	    			   "Copy cube" );
    const ioPixmap mergepm( GetDataFileName("mergeseis.png") );
    mergebut = manipgrp->addButton( mergepm, mCB(this,uiSeisFileMan,mergePush),
	    			    "Merge blocks of inlines into cube" );

    topgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
    selChg(0);
}


uiSeisFileMan::~uiSeisFileMan()
{
}


void uiSeisFileMan::ownSelChg()
{
    const bool is2d = ctio.ioobj && SeisTrcTranslator::is2D( *ctio.ioobj );
    copybut->setSensitive( is2d || 
	    		   (ctio.ioobj && ctio.ioobj->implExists(true)) );
    mergebut->setSensitive( !is2d );

    copybut->setPixmap( ioPixmap(GetDataFileName( is2d ? "man2d.png"
						: "copyobj.png" )) );
    const CallBack& cbcopy = mCB(this,uiSeisFileMan,copyPush);
    const CallBack& cb2d = mCB(this,uiSeisFileMan,man2DPush);
    const CallBack& oldcb = !is2d ? cb2d : cbcopy;
    const CallBack& newcb = is2d ? cb2d : cbcopy;
    if ( copybut->activated.cbs.indexOf(oldcb) >= 0 )
	copybut->activated.remove( oldcb );
    if ( copybut->activated.cbs.indexOf(newcb) < 0 )
	copybut->activated.notify( newcb );

    copybut->setToolTip( is2d ? "Manage lines" : "Copy cube" );
}


void uiSeisFileMan::mkFileInfo()
{
    if ( !ctio.ioobj )
    {
	infofld->setText( "" );
	return;
    }

    BufferString txt;
    const bool is2d = SeisTrcTranslator::is2D( *ctio.ioobj );
    if ( is2d )
    {
	BufferString fnm( ctio.ioobj->fullUserExpr(true) );
	Seis2DLineSet lset( fnm );
	txt += "Number of lines: "; txt += lset.nrLines();
    }


#define mRangeTxt(line) \
    txt += cs.hrg.start.line; txt += " - "; txt += cs.hrg.stop.line; \
    txt += " ["; txt += cs.hrg.step.line; txt += "]"

#define mZRangeTxt(memb) \
    txt += SI().zIsTime() ? mNINT(1000*memb) : memb

    CubeSampling cs;
    uiSeisIOObjInfo oinf( *ctio.ioobj, false );
    if ( !is2d && oinf.getRanges(cs) )
    {
	txt = "";
	if ( !mIsUndefInt(cs.hrg.stop.inl) )
	    { txt += "Inline range: "; mRangeTxt(inl); }
	if ( !mIsUndefInt(cs.hrg.stop.crl) )
	    { txt += "\nCrossline range: "; mRangeTxt(crl); }
	txt += "\nZ-range: "; 
	mZRangeTxt(cs.zrg.start); txt += " - "; mZRangeTxt(cs.zrg.stop); 
	txt += " ["; mZRangeTxt(cs.zrg.step); txt += "]";
    }

    if ( ctio.ioobj->pars().size() )
    {
	if ( ctio.ioobj->pars().hasKey("Type") )
	{ txt += "\nType: "; txt += ctio.ioobj->pars().find( "Type" ); }

	const char* optstr = "Optimized direction";
	if ( ctio.ioobj->pars().hasKey(optstr) )
	{ txt += "\nOptimized direction: ";
	    txt += ctio.ioobj->pars().find(optstr); }
    }

    txt += getFileInfo();

    infofld->setText( txt );
}


void uiSeisFileMan::copyPush( CallBacker* )
{
    if ( !ctio.ioobj ) return;
    mDynamicCastGet(const IOStream*,iostrm,ctio.ioobj)
    if ( !iostrm ) { pErrMsg("IOObj not IOStream"); return; }

    MultiID key( iostrm->key() );
    uiSeisImpCBVS dlg( this, iostrm );
    dlg.go();
    manipgrp->refreshList( key );
}


double uiSeisFileMan::getFileSize( const char* filenm )
{
    if ( File_isEmpty(filenm) ) return -1;

    double totalsz = 0;
    for ( int inr=0; ; inr++ )
    {
	BufferString fullnm( CBVSIOMgr::getFileName(filenm,inr) );
	if ( !File_exists(fullnm) ) break;
	
	totalsz += (double)File_getKbSize( fullnm );
    }

    return totalsz;
}


void uiSeisFileMan::mergePush( CallBacker* )
{
    if ( !ctio.ioobj ) return;

    MultiID key( ctio.ioobj->key() );
    uiMergeSeis dlg( this );
    dlg.go();
    manipgrp->refreshList( key );
}



class uiSeis2DMan : public uiDialog
{
public:

uiSeis2DMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup("Seismic file management",
				 "Manage 2D seismic lines",
				 "103.1.3"))
{
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
    linenames.sort();
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
    if ( !linenms.size() || !attribnms.size() )
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
    if ( !attribnms.size() || 
	    !uiMSG().askGoOn("All selected attributes will be removed.\n"
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
    if ( !linenms.size() ) return;

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
    if ( !attribnms.size() ) return;

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


void uiSeisFileMan::man2DPush( CallBacker* )
{
    const bool is2d = SeisTrcTranslator::is2D( *ctio.ioobj );
    if ( !is2d ) return;

    uiSeis2DMan dlg( this, *ctio.ioobj );
    dlg.go();
}
