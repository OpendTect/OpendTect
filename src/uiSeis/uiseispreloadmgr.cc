/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiseispreloadmgr.h"
#include "seisioobjinfo.h"
#include "seiscbvs.h"
#include "seispsioprov.h"
#include "seispreload.h"
#include "seis2ddata.h"
#include "strmprov.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "ascstream.h"
#include "ptrman.h"
#include "filepath.h"
#include "file.h"
#include "datapack.h"
#include "survinfo.h"
#include "preloads.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uisplitter.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uiseissel.h"
#include "uiselsurvranges.h"
#include "od_helpids.h"

const char* cannotloadstr = "Cannot load ";

uiSeisPreLoadMgr::uiSeisPreLoadMgr( uiParent* p )
    : uiDialog(p,Setup(tr("Seismic Data Pre-load Manager"),mNoDlgTitle,
			mODHelpKey(mSeisPreLoadMgrHelpID) ))
{
    setCtrlStyle( CloseOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries" );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( topgrp, "Manip buttons",
					     OD::Vertical );
    bgrp->attach( rightOf, listfld_ );
#   define mAddBut(s,fn) \
    new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), false )
    if ( has3d )
	mAddBut(tr("Add Cube"),cubeLoadPush);
    if ( has2d )
	mAddBut(tr("Add Lines"),linesLoadPush);
    if ( has3d )
    {
	if ( has2d )
	    mAddBut(tr("Add 3D Prestack data"),ps3DPush);
	else
	    mAddBut(tr("Add Prestack data"),ps3DPush);
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut(tr("Add 2D Prestack lines"),ps2DPush);
	else
	    mAddBut(tr("Add Prestack data"),ps2DPush);
    }
    mAddBut(tr("Unload Selected"),unloadPush);

    uiToolButton* opentb = new uiToolButton( topgrp, "openpreload",
	    tr("Retrieve pre-loads"), mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftAlignedBelow, listfld_ );
    uiToolButton* savetb = new uiToolButton( topgrp, "savepreload",
	    tr("Save pre-loads"), mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedBelow, listfld_ );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info" );
    infofld_->setPrefHeightInChar( 5 );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( topgrp );
    spl->addGroup( infogrp );

    postFinalise().notify( mCB(this,uiSeisPreLoadMgr,fullUpd) );
}


void uiSeisPreLoadMgr::fullUpd( CallBacker* )
{
    fillList();
}


void uiSeisPreLoadMgr::fillList()
{
    listfld_->setEmpty();
    StreamProvider::getPreLoadedIDs( ids_ );
    if ( ids_.isEmpty() ) return;

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const MultiID ky( ids_.get(idx) );
	PtrMan<IOObj> ioobj = IOM().get( ky );
	if ( !ioobj )
	{
	    Seis::PreLoader(ky).unLoad();
	    ids_.removeSingle( idx ); idx--;
	    continue;
	}
	listfld_->addItem( ioobj->name() );
    }

    listfld_->setCurrentItem( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( ids_.isEmpty() || selidx < 0 )
	{ infofld_->setText(""); return; }

    const MultiID ky( ids_.get(selidx).buf() );
    Seis::PreLoader spl( ky );
    PtrMan<IOObj> ioobj = spl.getIOObj();
    if ( !ioobj )
	{ infofld_->setText(spl.errMsg()); return; }

    SeisIOObjInfo ioinf( *ioobj );
    if ( !ioinf.isOK() )
	{ infofld_->setText("Internal error: IOObj not OK"); return; }
    const Seis::GeomType gt = ioinf.geomType();
    BufferStringSet fnms;
    StreamProvider::getPreLoadedFileNames( ky.buf(), fnms );
    if ( fnms.isEmpty() )
	{ infofld_->setText("No files"); return; }

    BufferString disptxt( "Data type: " ); disptxt += Seis::nameOf( gt );

    switch ( gt )
    {
	case Seis::Vol:
	break;
	case Seis::Line: {
	    BufferStringSet lnms; spl.getLineNames( lnms );
	    disptxt += getLinesText( lnms );
	} break;
	case Seis::VolPS: {
	    Interval<int> rg = spl.inlRange();
	    if ( !mIsUdf(rg.start) )
	    {
		disptxt += "\nInline range: ";
		disptxt += rg.start; disptxt += "-"; disptxt += rg.stop;
	    }
	} break;
	case Seis::LinePS:
	break;
    }

    float totmem; disptxt += getFilesText( fnms, totmem );
    totmem /= 1024; const int memmb = mNINT32(totmem);
    disptxt += "\nTotal memory in MB: "; disptxt += memmb;
    infofld_->setText( disptxt );
}


BufferString uiSeisPreLoadMgr::getLinesText( const BufferStringSet& lnms ) const
{
    BufferString txt;
    if ( lnms.isEmpty() )
	{ txt = "-"; return txt; }

    txt += "\nLine"; txt += lnms.size() < 2 ? ":" : "s:";
    txt += lnms.getDispString( -1, false );
    return txt;
}


BufferString uiSeisPreLoadMgr::getFilesText( const BufferStringSet& fnms,
						float& totmem ) const
{
    BufferString txt; const int nrfiles = fnms.size();

    FilePath fp( fnms.get(0) );
    txt += "\nDirectory: "; txt += fp.pathOnly();
    txt += "\nFile name"; if ( nrfiles > 1 ) txt += "s";
    txt += ": "; txt += fp.fileName();
    totmem = 0; const bool needdots = nrfiles > 4;
    for ( int idx=0; idx<nrfiles; idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	int dpid = StreamProvider::getPreLoadedDataPackID( fnm );
	totmem += DPM(DataPackMgr::BufID()).nrKBytesOf( dpid );

	if ( idx == 0 || (idx < nrfiles-1 && idx > 2) )
	    continue;

	fp.set( fnms.get(idx) );
	txt += " "; txt += fp.fileName();
	if ( needdots && idx == 2 )
	    txt += " ...";
    }
    if ( needdots )
	{ txt += " ("; txt += nrfiles; txt += " files)"; }

    return txt;
}



#define mCheckIOObjExistance( ioobj ) \
if ( !ioobj->implExists( true ) ) \
{ \
    BufferString msg = cannotloadstr; \
    msg += ioobj->name(); \
    uiMSG().error( msg.buf() ); \
    return; \
}



void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    ctio->ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    const char* id = spl.id().buf();
    if ( StreamProvider::isPreLoaded(id,true) )
    {
	if ( !uiMSG().askGoOn(tr("This cube is already pre-loaded.\n"
			      "Do you want to re-load?")) )
	    return;
	spl.unLoad();
    }

    uiTaskRunner taskrunner( this ); spl.setRunner( taskrunner );
    if ( !spl.loadVol() )
    {
	const uiString emsg = spl.errMsg();
	if ( emsg.isSet() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrSel2D : public uiDialog
{
public:

uiSeisPreLoadMgrSel2D( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Preload selection","Select 2D Dataset",
				 mODHelpKey(mSeisPreLoadMgrSel2DHelpID) ))
{
    IOObjContext ioctxt = uiSeisSel::ioContext( Seis::Line, true );
    dssel_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(true,false) );
    dssel_->selectionDone.notify( mCB(this,uiSeisPreLoadMgrSel2D,dsSel) );
    uiLabeledListBox* lllb = new uiLabeledListBox( this, "Line(s)",
				 OD::ChooseAtLeastOne,
				 uiLabeledListBox::AboveMid );
    lllb->attach( alignedBelow, dssel_ );
    linesel_ = lllb->box();
    postFinalise().notify( mCB(this,uiSeisPreLoadMgrSel2D,dsSel) );
}


void dsSel( CallBacker* cb )
{
    if ( !dssel_->ioobj(true) ) return;

    lnms_.erase();
    Seis2DDataSet ds( *dssel_->ioobj() );
    for ( int idx=0; idx<ds.nrLines(); idx++ )
	lnms_.add( ds.lineName(idx) );

    linesel_->setEmpty();
    linesel_->addItems( lnms_ );
    linesel_->chooseAll();
}


bool acceptOK( CallBacker* )
{
    if ( !dssel_->ioobj() )
	return false;

    lnms_.erase();
    linesel_->getChosen(lnms_);
    if ( lnms_.isEmpty() )
    {
	uiMSG().error( "Please select one or more lines" );
	return false;
    }

    return true;
}

    uiSeisSel*	dssel_;
    uiListBox*	linesel_;

    BufferStringSet	lnms_;
};


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadMgrSel2D dlg( this );
    if ( !dlg.go() ) return;

    mCheckIOObjExistance( dlg.dssel_->ioobj() );

    Seis::PreLoader spl( dlg.dssel_->ioobj()->key() );
    uiTaskRunner taskrunner( this ); spl.setRunner( taskrunner );
    if ( !spl.loadLines(dlg.lnms_) )
    {
	const uiString emsg = spl.errMsg();
	if ( emsg.isSet() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::ps3DPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisPS3D);
    ctio->ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    uiIOObjSelDlg dlg( this, *ctio, tr("Select data store/part to load") );
    dlg.setCaption( tr("Select data store") );
    uiSelNrRange* inlrgfld = new uiSelNrRange( dlg.selGrp()->getTopGroup(),
					uiSelNrRange::Inl, false );
    inlrgfld->attach( centeredBelow, dlg.selGrp()->getListField() );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    Interval<int> inlrg; assign(inlrg,inlrgfld->getRange());
    uiTaskRunner taskrunner( this ); spl.setRunner( taskrunner );
    if ( !spl.loadPS3D(&inlrg) )
    {
	const uiString emsg = spl.errMsg();
	if ( emsg.isSet() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrPS2DSel : public uiIOObjSelDlg
{
public:

uiSeisPreLoadMgrPS2DSel( uiParent* p, CtxtIOObj& ctio )
    : uiIOObjSelDlg( p, ctio, "Select data store" )
{
    setCaption( "Pre-load data" );
    uiLabeledListBox* llb = new uiLabeledListBox( selGrp(), "Line(s) to load",
			  OD::ChooseAtLeastOne, uiLabeledListBox::AboveMid );
    lnmsfld_ = llb->box();
    llb->attach( rightOf, selGrp()->getTopGroup() );
    selGrp()->selectionChanged.notify(
				mCB(this,uiSeisPreLoadMgrPS2DSel,dsSel) );
}

void dsSel( CallBacker* )
{
    lnmsfld_->setEmpty();
    if ( !ioObj() ) return;

    SeisIOObjInfo sii( *ioObj() );
    lnms_.erase();
    sii.getLineNames( lnms_ );
    lnmsfld_->addItems( lnms_ );
    lnmsfld_->chooseAll();
}

bool acceptOK( CallBacker* )
{
    if ( !ioObj() )
    {
	uiMSG().error( "Please select a 2D Prestack Data Store" );
	return false;
    }
    lnms_.erase();
    lnmsfld_->getChosen( lnms_ );
    if ( lnms_.isEmpty() )
    {
	uiMSG().error( "Please select one or more lines" );
	return false;
    }

    return true;
}

    uiListBox*	lnmsfld_;

    BufferStringSet lnms_;

};


void uiSeisPreLoadMgr::ps2DPush( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisPS2D);
    ctio->ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    uiSeisPreLoadMgrPS2DSel dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    Seis::PreLoader spl( dlg.ioObj()->key() );
    uiTaskRunner taskrunner( this ); spl.setRunner( taskrunner );
    if ( !spl.loadPS2D(dlg.lnms_) )
    {
	const uiString emsg = spl.errMsg();
	if ( emsg.isSet() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::unloadPush( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 ) return;

    BufferString msg( "Unload '" );
    msg += listfld_->textOfItem( selidx );
    msg += "'?\n(This will not delete the object from disk)";
    if ( !uiMSG().askGoOn( msg ) )
	return;

    Seis::PreLoader spl( MultiID(ids_.get(selidx)) );
    spl.unLoad();

    fillList();
    int newselidx = selidx;
    if ( newselidx >= ids_.size() )
	newselidx = ids_.size() - 1;
    if ( newselidx >= 0 )
	listfld_->setCurrentItem( newselidx );
    else
	selChg( 0 );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisPreLoadMgr::openPush( CallBacker* )
{
    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    ctio.fillDefault();
    uiIOObjSelDlg dlg( this, ctio, tr("Open pre-load settings") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( BufferString("Cannot open input file:\n",fnm) )

    ascistream astrm( strm,true );
    IOPar iop( astrm );
    if ( iop.isEmpty() )
	mErrRet( tr("No valid objects found") )

    uiTaskRunner taskrunner( this );
    Seis::PreLoader::load( iop, &taskrunner );
    fullUpd( 0 );
}


void uiSeisPreLoadMgr::savePush( CallBacker* )
{
    if ( ids_.isEmpty() ) return;

    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    uiIOObjSelDlg dlg( this, ctio, tr("Save pre-load settings") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj;
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( BufferString("Cannot open output file:\n",fnm) )

    IOPar alliop;
    for ( int iobj=0; iobj<ids_.size(); iobj++ )
    {
	const MultiID id( ids_.get(iobj).buf() );
	IOPar iop; const BufferString parkey( "Seis.", iobj );
	Seis::PreLoader spl( id ); spl.fillPar( iop );
	alliop.mergeComp( iop, parkey );
    }

    ascostream astrm( strm );
    if ( !astrm.putHeader("Pre-loads") )
	mErrRet( BufferString("Cannot write to output file:\n",fnm) )
    alliop.putTo( astrm );
}
