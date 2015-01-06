/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiseispreloadmgr.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "datapack.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "preloads.h"
#include "ptrman.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seispreload.h"
#include "seispsioprov.h"
#include "survinfo.h"

#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiselsurvranges.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
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

#define mAddBut(s,fn) \
    new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), false )

    if ( has3d )
    {
	mAddBut(tr("Add Cube"),cubeLoadPush);
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut(tr("Add 2D DataSet"),linesLoadPush);
	else
	    mAddBut(tr("Add DataSet"),linesLoadPush);
    }
/*
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
	    mAddBut(tr("Add 2D Prestack data"),ps2DPush);
	else
	    mAddBut(tr("Add Prestack data"),ps2DPush);
    }
*/
    mAddBut(tr("Unload Selected"),unloadPush);

    uiToolButton* savetb = new uiToolButton( topgrp, "save",
	    tr("Save pre-loads"), mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedBelow, listfld_ );
    uiToolButton* opentb = new uiToolButton( topgrp, "open",
	    tr("Retrieve pre-loads"), mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftOf, savetb );

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
    Seis::PLDM().getIDs( ids_ );
    if ( ids_.isEmpty() ) return;

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	const MultiID ky( ids_[idx] );
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
    if ( !ids_.validIdx(selidx) )
    { infofld_->setText(""); return; }

    Seis::PreLoader spl( ids_[selidx] );
    PtrMan<IOObj> ioobj = spl.getIOObj();
    if ( !ioobj )
    { infofld_->setText( spl.errMsg() ); return; }

    SeisIOObjInfo ioinf( *ioobj );
    if ( !ioinf.isOK() )
	{ infofld_->setText(tr("Internal error: IOObj not OK")); return; }

    BufferString disptxt;
    Seis::PLDM().getInfo( ids_[selidx], disptxt );
    infofld_->setText( disptxt );
}


#define mCheckIOObjExistance( ioobj ) \
if ( !ioobj->implExists( true ) ) \
{ \
    BufferString msg = cannotloadstr; \
    msg += ioobj->name(); \
    uiMSG().error( msg.buf() ); \
    return; \
}

class uiSeisPreLoadSel : public uiDialog
{
public:

uiSeisPreLoadSel( uiParent* p, Seis::GeomType geom )
    : uiDialog(p,uiDialog::Setup("",mNoDlgTitle,mNoHelpKey))
{
    setCaption( geom==Seis::Vol ? "Pre-load 3D Data" : "Pre-load 2D Data" );
    IOObjContext ctxt = uiSeisSel::ioContext( geom, true );
    uiSeisSel::Setup sssu( geom );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    seissel_ = new uiSeisSel( this, ctxt, sssu );
    seissel_->selectionDone.notify( mCB(this,uiSeisPreLoadSel,seisSel) );

    Seis::SelSetup selsu( geom ); selsu.multiline(true);
    subselfld_ = uiSeisSubSel::get( this, selsu );
    subselfld_->attach( alignedBelow, seissel_ );

    postFinalise().notify( mCB(this,uiSeisPreLoadSel,seisSel) );
}


void seisSel( CallBacker* )
{
    if ( !seissel_->ioobj() ) return;
    subselfld_->setInput( *seissel_->ioobj() );
}


bool acceptOK( CallBacker* )
{
    if ( !seissel_->ioobj() )
	return false;

    return true;
}

    uiSeisSel*		seissel_;
    uiSeisSubSel*	subselfld_;
};


void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Seis::Vol );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.seissel_->ioobj();
    mCheckIOObjExistance( ioobj );
// TODO: Check if already preloaded

    TrcKeyZSampling cs; dlg.subselfld_->getSampling( cs );
    Seis::PreLoader spl( ioobj->key() );
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );
    if ( !spl.loadVol(cs) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd(0);
}


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Seis::Line );
    if ( !dlg.go() ) return;

    mCheckIOObjExistance( dlg.seissel_->ioobj() );

    Seis::PreLoader spl( dlg.seissel_->key() );
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );

    mDynamicCastGet(uiSeis2DSubSel*,ss2d,dlg.subselfld_)
    if ( !ss2d ) return;

    TypeSet<Pos::GeomID> geomids;
    ss2d->selectedGeomIDs( geomids );
    TrcKeyZSampling cs;
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	ss2d->getSampling( cs, idx );
	if ( !spl.loadLine(geomids[idx],cs) )
	{
	    const uiString emsg = spl.errMsg();
	    if ( !emsg.isEmpty() )
		uiMSG().error( emsg );
	}
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
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );
    if ( !spl.loadPS3D(&inlrg) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


class uiSeisPreLoadMgrPS2DSel : public uiIOObjSelDlg
{ mODTextTranslationClass(uiSeisPreLoadMgrPS2DSel);
public:

uiSeisPreLoadMgrPS2DSel( uiParent* p, CtxtIOObj& ctio )
    : uiIOObjSelDlg( p, ctio, tr("Select data store") )
{
    setCaption( "Pre-load data" );
    uiLabeledListBox* llb = new uiLabeledListBox( selGrp(),
						  tr("Line(s) to load"),
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
	uiMSG().error( tr("Please select a 2D Prestack Data Store") );
	return false;
    }
    lnms_.erase();
    lnmsfld_->getChosen( lnms_ );
    if ( lnms_.isEmpty() )
    {
	uiMSG().error( tr("Please select one or more lines") );
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
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );
    if ( !spl.loadPS2D(dlg.lnms_) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd( 0 );
}


void uiSeisPreLoadMgr::unloadPush( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    if ( selidx < 0 ) return;

    uiString msg = tr("Unload '%1'?\n(This will not delete "
		      "the object from disk)")
		 .arg(listfld_->textOfItem( selidx ));
    if ( !uiMSG().askGoOn( msg ) )
	return;

    Seis::PreLoader spl( ids_[selidx] );
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
	mErrRet( tr("Cannot open input file:\n%1").arg(fnm) )

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
	mErrRet( tr("Cannot open output file:\n%1").arg(fnm) )

    IOPar alliop;
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	IOPar iop;
	Seis::PreLoader spl( ids_[idx] ); spl.fillPar( iop );
	alliop.mergeComp( iop, IOPar::compKey("Seis",idx) );
    }

    ascostream astrm( strm );
    if ( !astrm.putHeader("Pre-loads") )
	mErrRet( tr("Cannot write to output file:\n%1").arg(fnm) )
    alliop.putTo( astrm );
}
