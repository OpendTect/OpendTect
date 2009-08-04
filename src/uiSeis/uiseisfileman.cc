/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisfileman.cc,v 1.90 2009-08-04 12:23:16 cvsbert Exp $";


#include "uiseisfileman.h"
#include "uiseispsman.h"

#include "cbvsreadmgr.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "filegen.h"
#include "filepath.h"
#include "iopar.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "pixmap.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uiseisbrowser.h"
#include "uiseiscbvsimp.h"
#include "uiseisioobjinfo.h"
#include "uiseis2dgeom.h"
#include "uisplitter.h"
#include "uitextedit.h"


static const int cPrefWidth = 50;

uiSeisFileMan::uiSeisFileMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic data",
                                     "103.1.0").nrstatusflds(1),
	    	   SeisTrcTranslatorGroup::ioContext())
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

    browsebut = manipgrp->addButton( ioPixmap("browseseis.png"),
				     mCB(this,uiSeisFileMan,browsePush),
				     "Browse/edit this cube" );

    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );

    const bool have2d = SI().has2D();
    const bool have3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( this, "PS buttons" );
    if ( have3d )
    {
	const char* ttt = have2d ? "Manage 3D Pre-Stack data"
	    			 : "Manage Pre-Stack data";
	uiToolButton* tb = new uiToolButton( bgrp, ttt,
			   ioPixmap(have2d ? "man_ps3d.png" : "man_ps.png"),
			   mCB(this,uiSeisFileMan,manPS3D) );
	tb->setToolTip( ttt );
    }
    if ( have2d )
    {
	const char* ttt = have3d ? "Manage 2D Pre-Stack data"
	    			 : "Manage Pre-Stack data";
	uiToolButton* tb = new uiToolButton( bgrp, ttt,
			   ioPixmap(have3d ? "man_ps2d.png" : "man_ps.png"),
			   mCB(this,uiSeisFileMan,manPS2D) );
	tb->setToolTip( ttt );
    }
    bgrp->attach( centeredRightOf, selgrp );
    bgrp->displayFrame( true );

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


void uiSeisFileMan::ownSelChg()
{
    if ( !curioobj_ ) return;

    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    const bool is2d = curioobj_ && SeisTrcTranslator::is2D( *curioobj_ );
    manipgrp->useAlternative( cpym2dbut, is2d );
    manipgrp->useAlternative( mrgdmpbut, is2d );
    cpym2dbut->setSensitive( is2d || curimplexists_ );
    browsebut->setSensitive( !is2d && curimplexists_ );
}


void uiSeisFileMan::mkFileInfo()
{
    BufferString txt;
    SeisIOObjInfo oinf( curioobj_ );

    if ( oinf.isOK() )
    {

    const bool is2d = oinf.is2D();
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
	if ( curioobj_->pars().isTrue("Is Velocity") )
	{
	    txt += "\nVelocity Type: ";
	    const char* typstr = curioobj_->pars().find( "Velocity Type" );
	    txt += typstr ? typstr : "<unknown>";
	}
	if ( curioobj_->pars().hasKey(ZDomain::sKey()) )
	{ txt += "\nDomain: "; txt += curioobj_->pars().find(ZDomain::sKey()); }
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


static void doBrowse( uiParent* p, const IOObj* ioobj, const LineKey* lk,
		      bool is2d )
{
    if ( !ioobj ) return;

    uiSeisBrowser::Setup setup( ioobj->key(), lk ? Seis::Line : Seis::Vol );
    setup.readonly( ioobj->implReadOnly() );
    if ( lk )
	setup.linekey( *lk );
    uiSeisBrowser dlg( p, setup, is2d );
    dlg.go();
}


void uiSeisFileMan::browsePush( CallBacker* )
{
    doBrowse( this, curioobj_, 0, false );
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

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiLabeledListBox* lllb = new uiLabeledListBox( topgrp, "2D lines", false,
						  uiLabeledListBox::AboveMid );
    linelist = lllb->box();
    linelist->selectionChanged.notify( mCB(this,uiSeis2DMan,lineSel) );
    linelist->setMultiSelect(true);

    uiManipButGrp* linebutgrp = new uiManipButGrp( lllb );
    linebutgrp->addButton( uiManipButGrp::Rename, 
	    		   mCB(this,uiSeis2DMan,renameLine), "Rename line" );
    linebutgrp->attach( rightTo, linelist );

    uiLabeledListBox* allb = new uiLabeledListBox( topgrp, "Attributes", true,
						   uiLabeledListBox::AboveMid );
    attriblist = allb->box();
    attriblist->selectionChanged.notify( mCB(this,uiSeis2DMan,attribSel) );
    allb->attach( rightTo, lllb );

    uiManipButGrp* butgrp = new uiManipButGrp( allb );
    butgrp->addButton( uiManipButGrp::Rename,
	    	       mCB(this,uiSeis2DMan,renameAttrib), "Rename attribute" );
    butgrp->addButton( uiManipButGrp::Remove,
	    	       mCB(this,uiSeis2DMan,removeAttrib),
		       "Remove selected attribute(s)" );
    browsebut = butgrp->addButton( ioPixmap("browseseis.png"),
	    	       mCB(this,uiSeis2DMan,browsePush),
		       "Browse/edit this line" );
    butgrp->attach( rightTo, attriblist );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld = new uiTextEdit( botgrp, "File Info", true );
    infofld->setPrefHeightInChar( 8 );
    infofld->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

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
    uiListBox* lb = linelist;
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
    linelist->getSelectedItems( sellines );
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

    attriblist->empty();
    sharedattribs.sort();
    attriblist->addItems( sharedattribs );
    attriblist->setSelected( 0, true );
}


void attribSel( CallBacker* )
{
    infofld->setText( "" );
    BufferStringSet linenms, attribnms;
    linelist->getSelectedItems( linenms );
    attriblist->getSelectedItems( attribnms );
    if ( linenms.isEmpty() || attribnms.isEmpty() )
	{ browsebut->setSensitive( false ); return; }

    const LineKey linekey( linenms.get(0), attribnms.get(0) );
    const int lineidx = lineset->indexOf( linekey );
    if ( lineidx < 0 ) { pErrMsg("Huh"); return; }

    PosInfo::Line2DData l2dd;
    if ( !lineset->getGeometry(lineidx,l2dd) || l2dd.posns_.isEmpty() )
	return;

    const int sz = l2dd.posns_.size();
    BufferString txt( "Number of traces: " ); txt += sz;
    const PosInfo::Line2DPos& firstpos = l2dd.posns_[0];
    txt += "\nFirst trace: "; txt += firstpos.nr_;
    txt += " ("; txt += firstpos.coord_.x;
    txt += ","; txt += firstpos.coord_.y; txt += ")";
    const PosInfo::Line2DPos& lastpos = l2dd.posns_[sz-1];
    txt += "\nLast trace: "; txt += lastpos.nr_;
    txt += " ("; txt += lastpos.coord_.x;
    txt += ","; txt += lastpos.coord_.y; txt += ")";
    txt += "\nZ-range: "; mZRangeTxt(l2dd.zrg_.start); txt += " - ";
    mZRangeTxt(l2dd.zrg_.stop);
    txt += " ["; mZRangeTxt(l2dd.zrg_.step); txt += "]";

    const IOPar& iopar = lineset->getInfo( lineidx );
    BufferString fname(iopar.find(sKey::FileName) );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    fname = fp.fullPath();

    txt += "\nLocation: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; 
    txt += uiObjFileMan::getFileSizeString( File_getKbSize(fname) );
    infofld->setText( txt );

    browsebut->setSensitive( true );
}


void browsePush( CallBacker* )
{
    const LineKey lk( linelist->getText(), attriblist->getText());
    doBrowse( this, objinfo->ioObj(), &lk, true );
}


void removeAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attriblist->getSelectedItems( attribnms );
    if ( attribnms.isEmpty()
      || !uiMSG().askRemove("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    BufferStringSet sellines;
    linelist->getSelectedItems( sellines );
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
    linelist->getSelectedItems( linenms );
    if ( linenms.isEmpty() ) return;

    const char* linenm = linenms.get(0);
    BufferString newnm;
    if ( !rename(linenm,newnm) ) return;
    
    if ( linelist->isPresent(newnm) )
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
    attriblist->getSelectedItems( attribnms );
    if ( attribnms.isEmpty() ) return;

    const char* attribnm = attribnms.get(0);
    BufferString newnm;
    if ( !rename(attribnm,newnm) ) return;

    if ( attriblist->isPresent(newnm) )
    {
	uiMSG().error( "Attribute name already in use" );
	return;
    }

    BufferStringSet sellines;
    linelist->getSelectedItems( sellines );
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

    uiListBox*		linelist;
    uiListBox*		attriblist;
    uiTextEdit*		infofld;
    uiToolButton*	browsebut;

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


void uiSeisFileMan::manPS3D( CallBacker* )
{
    uiSeisPreStackMan dlg( this, false );
    dlg.go();
}


void uiSeisFileMan::manPS2D( CallBacker* )
{
    uiSeisPreStackMan dlg( this, true );
    dlg.go();
}
