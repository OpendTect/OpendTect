/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisfileman.cc,v 1.99 2009-12-15 12:20:18 cvsbert Exp $";


#include "uiseisfileman.h"
#include "uiseis2dfileman.h"
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
#include "seis2dlinemerge.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uiseisbrowser.h"
#include "uiseiscbvsimp.h"
#include "uiseisioobjinfo.h"
#include "uiseis2dgeom.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uitaskrunner.h"

static const int cPrefWidth = 50;

Notifier<uiSeis2DFileMan>* uiSeis2DFileMan::fieldsCreated()
{
    static Notifier<uiSeis2DFileMan> FieldsCreated(0);
    return &FieldsCreated;
}


uiSeisFileMan::uiSeisFileMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup("Seismic file management",
                                     "Manage seismic data",
                                     "103.1.0").nrstatusflds(1),
	    	   SeisTrcTranslatorGroup::ioContext())
    , is2d_(is2d)
{
    ctxt_.trglobexpr = is2d_ ? "2D" : "CBVS";
    createDefaultUI();
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
	BufferString fnm( curioobj_->fullUserExpr(true) );
	Seis2DLineSet lset( fnm );
	txt += "Number of lines: "; txt += lset.nrLines();
    }

#define mRangeTxt(line) \
    txt += cs.hrg.start.line; txt += " - "; txt += cs.hrg.stop.line; \
    txt += " ["; txt += cs.hrg.step.line; txt += "]"

    const bool issidomain = ZDomain::isSIDomain( curioobj_->pars() );
    const bool zistm = (SI().zIsTime() && issidomain)
		    || (!SI().zIsTime() && !issidomain);

#define mAddZRangeTxt(memb) \
    txt += zistm ? mNINT(1000*memb) : memb

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



uiSeis2DFileMan::uiSeis2DFileMan( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,uiDialog::Setup("Seismic line data management",
				 "Manage 2D seismic lines",
				 "103.1.3"))
    , issidomain(ZDomain::isSIDomain( ioobj.pars() ))
    , zistm((SI().zIsTime() && issidomain) || (!SI().zIsTime() && !issidomain))
{
    setCtrlStyle( LeaveOnly );

    objinfo_ = new uiSeisIOObjInfo( ioobj );
    lineset_ = new Seis2DLineSet( ioobj.fullUserExpr(true) );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiLabeledListBox* lllb = new uiLabeledListBox( topgrp, "2D lines", false,
						  uiLabeledListBox::AboveMid );
    linefld_ = lllb->box();
    linefld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,lineSel) );
    linefld_->setMultiSelect(true);

    linegrp_ = new uiManipButGrp( lllb );
    linegrp_->addButton( uiManipButGrp::Rename, 
	    		 mCB(this,uiSeis2DFileMan,renameLine), "Rename line");
    linegrp_->addButton( ioPixmap("mergelines.png"),
	    		 mCB(this,uiSeis2DFileMan,mergeLines), "Merge lines");
    linegrp_->attach( rightOf, linefld_ );

    uiLabeledListBox* allb = new uiLabeledListBox( topgrp, "Attributes", true,
						   uiLabeledListBox::AboveMid );
    attrfld_ = allb->box();
    attrfld_->selectionChanged.notify( mCB(this,uiSeis2DFileMan,attribSel) );
    allb->attach( rightOf, lllb );

    attrgrp_ = new uiManipButGrp( allb );
    attrgrp_->addButton( uiManipButGrp::Rename,
	    	       mCB(this,uiSeis2DFileMan,renameAttrib),
		       "Rename attribute" );
    attrgrp_->addButton( uiManipButGrp::Remove,
	    		mCB(this,uiSeis2DFileMan,removeAttrib),
			"Remove selected attribute(s)" );
    browsebut_ = attrgrp_->addButton( ioPixmap("browseseis.png"),
	    	       mCB(this,uiSeis2DFileMan,browsePush),
		       "Browse/edit this line" );
    attrgrp_->attach( rightOf, attrfld_ );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    fillLineBox();

    fieldsCreated()->trigger( this );
    lineSel(0);
}


uiSeis2DFileMan::~uiSeis2DFileMan()
{
    delete objinfo_;
    delete lineset_;
}


void uiSeis2DFileMan::fillLineBox()
{
    uiListBox* lb = linefld_;
    const int curitm = lb->size() ? lb->currentItem() : 0;
    BufferStringSet linenames;
    objinfo_->getLineNames( linenames );
    lb->empty();
    lb->addItems( linenames );
    lb->setSelected( curitm );
}


void uiSeis2DFileMan::lineSel( CallBacker* )
{
    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    BufferStringSet sharedattribs;
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	BufferStringSet attrs;
	objinfo_->getAttribNamesForLine( sellines.get(idx), attrs );
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

    attrfld_->empty();
    sharedattribs.sort();
    attrfld_->addItems( sharedattribs );
    attrfld_->setSelected( 0, true );
}


void uiSeis2DFileMan::attribSel( CallBacker* )
{
    infofld_->setText( "" );
    BufferStringSet linenms, attribnms;
    linefld_->getSelectedItems( linenms );
    attrfld_->getSelectedItems( attribnms );
    if ( linenms.isEmpty() || attribnms.isEmpty() )
	{ browsebut_->setSensitive( false ); return; }

    const LineKey linekey( linenms.get(0), attribnms.get(0) );
    const int lineidx = lineset_->indexOf( linekey );
    if ( lineidx < 0 ) { pErrMsg("Huh"); return; }

    PosInfo::Line2DData l2dd;
    if ( !lineset_->getGeometry(lineidx,l2dd) || l2dd.posns_.isEmpty() )
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
    txt += "\nZ-range: "; mAddZRangeTxt(l2dd.zrg_.start); txt += " - ";
    mAddZRangeTxt(l2dd.zrg_.stop);
    txt += " ["; mAddZRangeTxt(l2dd.zrg_.step); txt += "]";

    SeisIOObjInfo sobinf( objinfo_->ioObj() );
    const int nrcomp = sobinf.nrComponents( linekey );
    if ( nrcomp > 1 )
	{ txt += "\nNumber of components: "; txt += nrcomp; }

    const IOPar& iopar = lineset_->getInfo( lineidx );
    BufferString fname(iopar.find(sKey::FileName) );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    fname = fp.fullPath();

    txt += "\nLocation: "; txt += fp.pathOnly();
    txt += "\nFile name: "; txt += fp.fileName();
    txt += "\nFile size: "; 
    txt += uiObjFileMan::getFileSizeString( File_getKbSize(fname) );
    const char* timestr = File_getTime( fname );
    if ( timestr ) { txt += "\nLast modified: "; txt += timestr; }
    infofld_->setText( txt );

    browsebut_->setSensitive( true );
}


void uiSeis2DFileMan::browsePush( CallBacker* )
{
    const LineKey lk( linefld_->getText(), attrfld_->getText());
    doBrowse( this, objinfo_->ioObj(), &lk, true );
}


void uiSeis2DFileMan::removeAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attrfld_->getSelectedItems( attribnms );
    if ( attribnms.isEmpty()
      || !uiMSG().askRemove("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linename = sellines.get(idx);
	for ( int ida=0; ida<attribnms.size(); ida++ )
	{
	    LineKey linekey( linename, attribnms.get(ida) );
	    if ( !lineset_->remove(linekey) )
		uiMSG().error( "Could not remove attribute" );
	}
    }

    fillLineBox();
}


bool uiSeis2DFileMan::rename( const char* oldnm, BufferString& newnm )
{
    BufferString titl( "Rename '" );
    titl += oldnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(oldnm) );
    if ( !dlg.go() ) return false;
    newnm = dlg.text();
    return newnm != oldnm;
}


void uiSeis2DFileMan::renameLine( CallBacker* )
{
    BufferStringSet linenms;
    linefld_->getSelectedItems( linenms );
    if ( linenms.isEmpty() ) return;

    const char* linenm = linenms.get(0);
    BufferString newnm;
    if ( !rename(linenm,newnm) ) return;
    
    if ( linefld_->isPresent(newnm) )
    {
	uiMSG().error( "Linename already in use" );
	return;
    }

    if ( !lineset_->renameLine( linenm, newnm ) )
    {
	uiMSG().error( "Could not rename line" );
	return;
    }

    fillLineBox();
}


void uiSeis2DFileMan::renameAttrib( CallBacker* )
{
    BufferStringSet attribnms;
    attrfld_->getSelectedItems( attribnms );
    if ( attribnms.isEmpty() ) return;

    const char* attribnm = attribnms.get(0);
    BufferString newnm;
    if ( !rename(attribnm,newnm) ) return;

    if ( attrfld_->isPresent(newnm) )
    {
	uiMSG().error( "Attribute name already in use" );
	return;
    }

    BufferStringSet sellines;
    linefld_->getSelectedItems( sellines );
    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const char* linenm = sellines.get(idx);
	LineKey oldlk( linenm, attribnm );
	if ( !lineset_->rename(oldlk,LineKey(linenm,newnm)) )
	{
	    BufferString err( "Could not rename attribute: " );
	    err += oldlk;
	    uiMSG().error( err );
	    continue;
	}
    }

    lineSel(0);
}


class uiSeis2DFileManMergeDlg : public uiDialog
{
public:

uiSeis2DFileManMergeDlg( uiParent* p, const uiSeisIOObjInfo& objinf,
			 const BufferStringSet& sellns )
    : uiDialog(p,Setup("Merge lines","Merge two lines into a new one",
		       mTODOHelpID) )
    , objinf_(objinf)
{
    BufferStringSet lnms; objinf_.getLineNames( lnms );
    uiLabeledComboBox* lcb1 = new uiLabeledComboBox( this, lnms, "First line" );
    uiLabeledComboBox* lcb2 = new uiLabeledComboBox( this, lnms, "Add" );
    lcb2->attach( alignedBelow, lcb1 );
    ln1fld_ = lcb1->box(); ln2fld_ = lcb2->box();
    ln1fld_->setCurrentItem( sellns.get(0) );
    ln2fld_->setCurrentItem( sellns.get(1) );

    static const char* mrgopts[]
	= { "Match trace numbers", "Match coordinates", "Bluntly append", 0 };
    mrgoptfld_ = new uiGenInput( this, "Merge method",
	    			 StringListInpSpec(mrgopts) );
    mrgoptfld_->attach( alignedBelow, lcb2 );
    mrgoptfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
    stckfld_ = new uiGenInput( this, "Duplicate positions",
	    			BoolInpSpec(true,"Stack","Use first") );
    stckfld_->attach( alignedBelow, mrgoptfld_ );
    renumbfld_ = new uiGenInput( this, "Renumber",
	    	BoolInpSpec(true,"Continue first line", "New numbering") );
    renumbfld_->setWithCheck( true );
    renumbfld_->setChecked( true );
    renumbfld_->attach( alignedBelow, stckfld_ );
    nrdeffld_ = new uiGenInput( this, "Start/step numbers", IntInpSpec(1),
	    			IntInpSpec(1) );
    nrdeffld_->attach( alignedBelow, renumbfld_ );
    double defsd = SI().crlDistance() / 2;
    if ( SI().xyInFeet() ) defsd *= mToFeetFactorD;
    snapdistfld_ = new uiGenInput( this, "Snap distance", DoubleInpSpec(defsd));
    snapdistfld_->attach( alignedBelow, nrdeffld_ );

    outfld_ = new uiGenInput( this, "New line name", StringInpSpec() );
    outfld_->attach( alignedBelow, snapdistfld_ );

    finaliseDone.notify( mCB(this,uiSeis2DFileManMergeDlg,initWin) );
}

void initWin( CallBacker* )
{
    optSel(0);
    renumbfld_->valuechanged.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
    renumbfld_->checked.notify( mCB(this,uiSeis2DFileManMergeDlg,optSel) );
}

void optSel( CallBacker* )
{
    const int opt = mrgoptfld_->getIntValue();
    const bool dorenumb = renumbfld_->isChecked()
		       && !renumbfld_->getBoolValue();
    stckfld_->display( opt < 2 );
    renumbfld_->display( opt > 0 );
    nrdeffld_->display( opt > 0 && dorenumb );
    snapdistfld_->display( opt == 1 );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* )
{
    const char* outnm = outfld_->text();
    if ( !outnm || !*outnm )
	mErrRet( "Please enter a name for the merged line" );

    BufferStringSet lnms; objinf_.getLineNames( lnms );
    if ( lnms.isPresent( outnm ) )
	mErrRet( "Output line name already in Line Set" );

    Seis2DLineMerger lmrgr( objinf_.ioObj()->key() );
    lmrgr.lnm1_ = ln1fld_->text();
    lmrgr.lnm2_ = ln2fld_->text();
    if ( lmrgr.lnm1_ == lmrgr.lnm2_ )
	mErrRet( "Respectfully refusing to merge a line with itself" );

    lmrgr.opt_ = (Seis2DLineMerger::Opt)mrgoptfld_->getIntValue();
    lmrgr.renumber_ = lmrgr.opt_ != Seis2DLineMerger::MatchTrcNr
		   && renumbfld_->isChecked();

    if ( lmrgr.renumber_ )
    {
	lmrgr.numbering_.start = nrdeffld_->getIntValue(0);
	lmrgr.numbering_.step = nrdeffld_->getIntValue(1);
    }

    if ( lmrgr.opt_ == Seis2DLineMerger::MatchCoords )
    {
	lmrgr.snapdist_ = snapdistfld_->getdValue();
	if ( mIsUdf(lmrgr.snapdist_) || lmrgr.snapdist_ < 0 )
	    mErrRet( "Please specify a valid snap distance" );
    }

    uiTaskRunner tr( this );
    return tr.execute( lmrgr );
}

    const uiSeisIOObjInfo&	objinf_;

    uiComboBox*			ln1fld_;
    uiComboBox*			ln2fld_;
    uiGenInput*			mrgoptfld_;
    uiGenInput*			stckfld_;
    uiGenInput*			renumbfld_;
    uiGenInput*			nrdeffld_;
    uiGenInput*			snapdistfld_;
    uiGenInput*			outfld_;

};


void uiSeis2DFileMan::mergeLines( CallBacker* )
{
    if ( linefld_->size() < 2 ) return;

    BufferStringSet sellnms; int firstsel = -1;
    for ( int idx=0; idx<linefld_->size(); idx++ )
    {
	if ( linefld_->isSelected(idx) )
	{
	    sellnms.add( linefld_->textOfItem(idx) );
	    if ( firstsel < 0 ) firstsel = idx;
	    if ( sellnms.size() > 1 ) break;
	}
    }
    if ( firstsel < 0 )
	{ firstsel = 0; sellnms.add( linefld_->textOfItem(0) ); }
    if ( sellnms.size() == 1 )
    {
	if ( firstsel >= linefld_->size() )
	    firstsel = -1;
	sellnms.add( linefld_->textOfItem(firstsel+1) );
    }

    uiSeis2DFileManMergeDlg dlg( this, *objinfo_, sellnms );
    if ( dlg.go() )
	fillLineBox();
}
