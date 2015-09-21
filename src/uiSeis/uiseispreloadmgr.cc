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
#include "uiscaler.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiselsurvranges.h"
#include "uisplitter.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

using namespace Seis;

const char* cannotloadstr = "Cannot load ";

uiSeisPreLoadMgr::uiSeisPreLoadMgr( uiParent* p )
    : uiDialog(p,Setup(tr("Seismic Data Pre-load Manager"),mNoDlgTitle,
			mODHelpKey(mSeisPreLoadMgrHelpID) ))
{
    setCtrlStyle( CloseOnly );
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    listfld_ = new uiListBox( topgrp, "Loaded entries", OD::ChooseZeroOrMore );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( listfld_, "Manip buttons",
					     OD::Vertical );
    bgrp->attach( rightOf, listfld_->box() );

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
    mAddBut(tr("Unload Checked"),unloadPush);

    uiToolButton* savetb = new uiToolButton( topgrp, "save",
	    tr("Save pre-loads"), mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedBelow, listfld_ );
    uiToolButton* opentb = new uiToolButton( topgrp, "open",
	    tr("Retrieve pre-loads"), mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftOf, savetb );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info" );
    infofld_->setPrefHeightInChar( 8 );

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
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    if ( entries.isEmpty() ) return;

    for ( int idx=0; idx<entries.size(); idx++ )
	listfld_->addItem( entries[idx]->name_ );

    listfld_->setCurrentItem( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    if ( !entries.validIdx(selidx) )
    { infofld_->setText(""); return; }

    const MultiID mid = entries[selidx]->mid_;
    SeisIOObjInfo ioinf( mid );
    if ( !ioinf.isOK() )
	{ infofld_->setText(tr("Internal error: IOObj not OK")); return; }

    BufferString disptxt;
    PLDM().getInfo( mid, entries[selidx]->geomid_, disptxt );
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
{ mODTextTranslationClass(uiSeisPreLoadSel)
public:

uiSeisPreLoadSel( uiParent* p, GeomType geom )
    : uiDialog(p,uiDialog::Setup("",mNoDlgTitle,mNoHelpKey).nrstatusflds(1))
{
    setCaption( geom==Vol ? tr("Pre-load 3D Data")
				: tr("Pre-load 2D Data") );
    IOObjContext ctxt = uiSeisSel::ioContext( geom, true );
    uiSeisSel::Setup sssu( geom );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    seissel_ = new uiSeisSel( this, ctxt, sssu );
    seissel_->selectionDone.notify( mCB(this,uiSeisPreLoadSel,seisSel) );

    SelSetup selsu( geom ); selsu.multiline(true);
    subselfld_ = uiSeisSubSel::get( this, selsu );
    subselfld_->selChange.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
    subselfld_->attach( alignedBelow, seissel_ );

    scalerfld_ = new uiScaler( this, 0, true );
    scalerfld_->attach( alignedBelow, subselfld_ );

    typefld_ = new uiGenInput( this, tr("Load as"),
		StringListInpSpec(DataCharacteristics::UserTypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
    typefld_->setValue( (int)DataCharacteristics::Auto );
    typefld_->attach( alignedBelow, scalerfld_ );

#ifndef __debug__
    scalerfld_->setSensitive( true );
    typefld_->setSensitive( true );
#endif

    postFinalise().notify( mCB(this,uiSeisPreLoadSel,seisSel) );
}


void seisSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    typefld_->setValue( 0 );
    subselfld_->setInput( *ioobj );
    updateEstUsage();
}


void selChangeCB( CallBacker* )
{
    updateEstUsage();
}


void getDataChar( DataCharacteristics& dc )
{
    const DataCharacteristics::UserType type( (DataCharacteristics::UserType)
					       typefld_->getIntValue() );
    if ( type == DataCharacteristics::Auto )
    {
	SeisIOObjInfo info( seissel_->ioobj(true) );
	info.getDataChar( dc );
    }
    else
	dc = DataCharacteristics( type );
}


void updateEstUsage()
{
    SeisIOObjInfo info(seissel_->ioobj() );
    const int nrcomp = info.nrComponents();

    BufferString infotxt = "Data format on disk: ";
    if ( nrcomp > 0 )
    {
	DataCharacteristics dc; info.getDataChar( dc );
	const FixedString usertypestr =
	    DataCharacteristics::getUserTypeString( dc.userType() );
	if ( usertypestr.size() > 4 )
	    infotxt += usertypestr.buf() + 4;

	infotxt += ". Estimated memory usage: "; getDataChar( dc );
	const od_int64 nrs = subselfld_->expectedNrSamples();
	const od_int64 nrt = subselfld_->expectedNrTraces();
	const od_int64 nrbytes = nrcomp * nrs * nrt * dc.nrBytes();
	infotxt.add( File::getFileSizeString( nrbytes/1024 ) );
    }
    else
	infotxt.add( "?" );

    toStatusBar( infotxt );
}


bool acceptOK( CallBacker* )
{
    return seissel_->ioobj();
}

    uiSeisSel*		seissel_;
    uiSeisSubSel*	subselfld_;
    uiScaler*		scalerfld_;
    uiGenInput*		typefld_;
};


void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Vol );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.seissel_->ioobj();
    mCheckIOObjExistance( ioobj );
    const MultiID key = ioobj->key();
    if ( PLDM().isPresent(key) )
    {
	uiString msg( ioobj->name() );
	msg.append( " is already preloaded.\nDo you want to reload the cube?" );
	if ( !uiMSG().askGoOn(msg) ) return;

	PreLoader spl( key );
	spl.unLoad();
    }

    TrcKeyZSampling tkzs; dlg.subselfld_->getSampling( tkzs );
    DataCharacteristics dc; dlg.getDataChar( dc );
    PreLoader spl( key );
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );
    if ( !spl.load(tkzs,dc.userType(),dlg.scalerfld_->getScaler()) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd(0);
}


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Line );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.seissel_->ioobj();
    mCheckIOObjExistance( ioobj );

    mDynamicCastGet(uiSeis2DSubSel*,ss2d,dlg.subselfld_)
    if ( !ss2d ) return;

    uiTaskRunner taskrunner( this );
    TrcKeyZSampling tkzs;
    DataCharacteristics dc; dlg.getDataChar( dc );
    TypeSet<Pos::GeomID> geomids;
    ss2d->selectedGeomIDs( geomids );
    const MultiID key = dlg.seissel_->key();
    TypeSet<Pos::GeomID> loadedgeomids;
    for ( int idx=0; idx<geomids.size(); idx++ )
	if ( PLDM().isPresent(key,geomids[idx]) )
	    loadedgeomids += geomids[idx];

    bool skiploadedgeomids = false;
    if ( !loadedgeomids.isEmpty() )
    {
	uiString msg( IOM().nameOf(key) );
	msg.append( " dataset for " );
	msg.append( loadedgeomids.size()>1 ? "lines " : "line " );
	for ( int idx=0; idx<loadedgeomids.size(); idx++ )
	{
	    msg.append( Survey::GM().getName(loadedgeomids[idx]) );
	    if ( idx < loadedgeomids.size()-1 )
		msg.append( ", " );
	}

	msg.append( " is already preloaded. Do you want to reload?" );
	skiploadedgeomids = !uiMSG().askGoOn( msg );
    }

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const Pos::GeomID& geomid = geomids[idx];
	if ( PLDM().isPresent(key,geomid) )
	{
	    if ( skiploadedgeomids ) continue;
	    PreLoader spl( key, geomid );
	    spl.unLoad();
	}

	ss2d->getSampling( tkzs, geomid );
	tkzs.hsamp_.setLineRange( Interval<int>(geomid,geomid) );

	PreLoader spl( key, geomid, &taskrunner );
	if ( !spl.load(tkzs,dc.userType(),dlg.scalerfld_->getScaler()) )
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
    ctio->ctxt_.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    uiIOObjSelDlg dlg( this, *ctio, tr("Select data store/part to load") );
    dlg.setCaption( tr("Select data store") );
    uiSelNrRange* inlrgfld = new uiSelNrRange( dlg.selGrp()->getTopGroup(),
					uiSelNrRange::Inl, false );
    inlrgfld->attach( centeredBelow, dlg.selGrp()->getListField() );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    PreLoader spl( dlg.ioObj()->key() );
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
{ mODTextTranslationClass(uiSeisPreLoadMgrPS2DSel)
public:

uiSeisPreLoadMgrPS2DSel( uiParent* p, CtxtIOObj& ctio )
    : uiIOObjSelDlg( p, ctio, tr("Select data store") )
{
    setCaption( "Pre-load data" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Line(s) to load"),
			 uiListBox::AboveMid );
    lnmsfld_ = new uiListBox( selGrp(), su );
    lnmsfld_->attach( rightOf, selGrp()->getTopGroup() );
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
    ctio->ctxt_.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    uiSeisPreLoadMgrPS2DSel dlg( this, *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    mCheckIOObjExistance( dlg.ioObj() );

    PreLoader spl( dlg.ioObj()->key() );
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
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    TypeSet<int> selitms; listfld_->getChosen( selitms );

    uiString msg = tr("Unload checked items?\n(This will not delete "
		      "the data from disk)");
    if ( !uiMSG().askGoOn( msg ) )
	return;

    for ( int idx=selitms.size()-1; idx>=0; idx-- )
    {
	const int selidx = selitms[idx];
	PreLoader spl( entries[selidx]->mid_, entries[selidx]->geomid_ );
	spl.unLoad();
    }

    fillList();
    selChg( 0 );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisPreLoadMgr::openPush( CallBacker* )
{
    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    ctio.fillDefault();
    uiIOObjSelDlg dlg( this, ctio, tr("Open pre-load settings") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj_;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open input file:\n%1").arg(fnm) )

    ascistream astrm( strm,true );
    IOPar iop( astrm );
    if ( iop.isEmpty() )
	mErrRet( tr("No valid objects found") )

    uiTaskRunner taskrunner( this );
    PreLoader::load( iop, &taskrunner );
    fullUpd( 0 );
}


void uiSeisPreLoadMgr::savePush( CallBacker* )
{
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    if ( entries.isEmpty() ) return;

    CtxtIOObj ctio( PreLoadsTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = false;
    uiIOObjSelDlg dlg( this, ctio, tr("Save pre-load settings") );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    const BufferString fnm( dlg.ioObj()->fullUserExpr(true) );
    delete ctio.ioobj_;
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open output file:\n%1").arg(fnm) )

    IOPar alliop;
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	IOPar iop;
	PreLoader spl( entries[idx]->mid_, entries[idx]->geomid_ );
	spl.fillPar( iop );
	alliop.mergeComp( iop, IOPar::compKey("Seis",idx) );
    }

    ascostream astrm( strm );
    if ( !astrm.putHeader("Pre-loads") )
	mErrRet( tr("Cannot write to output file:\n%1").arg(fnm) )
    alliop.putTo( astrm );
}
