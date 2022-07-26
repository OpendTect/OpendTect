/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/


#include "uiseispreloadmgr.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapack.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "preloads.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seispreload.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "statrand.h"
#include "survinfo.h"

#include "uiaxishandler.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimapperrangeeditor.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiselsimple.h"
#include "uiselsurvranges.h"
#include "uisplitter.h"
#include "uistrings.h"
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
    listfld_ = new uiListBox( topgrp, "Loaded entries", OD::ChooseAtLeastOne );
    listfld_->selectionChanged.notify( mCB(this,uiSeisPreLoadMgr,selChg) );
    topgrp->setHAlignObj( listfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    uiButtonGroup* bgrp = new uiButtonGroup( listfld_, "Manip buttons",
					     OD::Vertical );
    bgrp->attach( rightOf, listfld_->box() );

#define mAddBut(s,fn,ic) { \
    uiButton* but = new uiPushButton( bgrp, s, mCB(this,uiSeisPreLoadMgr,fn), \
					false ); \
    but->setIcon( ic ); }

    if ( has3d )
    {
	mAddBut(tr("Load Cube"), cubeLoadPush, "seismiccube" )
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut( tr("Load 2D DataSet"), linesLoadPush, "seismicline2d" )
	else
	    mAddBut( tr("Load DataSet"), linesLoadPush, "seismicline2d" )
    }
    mAddBut( tr("Unload"), unloadPush, "unload" );

    uiToolButton* savetb = new uiToolButton( listfld_, "save",
	    tr("Save pre-loads"), mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedAbove, listfld_->box() );
    uiToolButton* opentb = new uiToolButton( listfld_, "open",
	    tr("Retrieve pre-loads"), mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftOf, savetb );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info" );
    infofld_->setPrefHeightInChar( 8 );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( topgrp );
    spl->addGroup( infogrp );

    postFinalize().notify( mCB(this,uiSeisPreLoadMgr,fullUpd) );
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
	listfld_->addItem( toUiString(entries[idx]->name_) );

    listfld_->setCurrentItem( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    if ( !entries.validIdx(selidx) )
    {
	infofld_->setText( "No pre-loaded seismics" );
	return;
    }

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
    uiString msg = toUiString("%1 %2").arg(toUiString(cannotloadstr)). \
		   arg(ioobj->uiName()); \
    uiMSG().error( msg ); \
    return; \
}

void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Vol, MultiID::udf() );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );
    const MultiID key = ioobj->key();
    if ( PLDM().isPresent(key) )
    {
	uiString msg( ioobj->uiName() );
	msg.append( " is already preloaded."
		    "\n\nDo you want to reload the cube?" );
	if ( !uiMSG().askGoOn(msg) ) return;

	PreLoader spl( key );
	spl.unLoad();
    }

    TrcKeyZSampling tkzs; dlg.getSampling( tkzs );
    DataCharacteristics dc; dlg.getDataChar( dc );
    PreLoader spl( key );
    uiTaskRunner taskrunner( this ); spl.setTaskRunner( taskrunner );
    if ( !spl.load(tkzs,dc.userType(),dlg.getScaler()) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd(0);
}


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Line, MultiID::udf() );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );

    TrcKeyZSampling tkzs;
    DataCharacteristics dc; dlg.getDataChar( dc );
    TypeSet<Pos::GeomID> geomids;
    dlg.selectedGeomIDs( geomids );
    const MultiID key = ioobj->key();
    TypeSet<Pos::GeomID> loadedgeomids;
    for ( int idx=0; idx<geomids.size(); idx++ )
	if ( PLDM().isPresent(key,geomids[idx]) )
	    loadedgeomids += geomids[idx];

    bool skiploadedgeomids = false;
    if ( !loadedgeomids.isEmpty() )
    {
	uiString msg( toUiString(IOM().nameOf(key)) );
	msg.append( " dataset for " );
	msg.append( loadedgeomids.size()>1 ? "lines " : "line " );
	for ( int idx=0; idx<loadedgeomids.size(); idx++ )
	{
	    msg.append( Survey::GM().getName(loadedgeomids[idx]) );
	    if ( idx < loadedgeomids.size()-1 )
		msg.append( ", " );
	}

	msg.append( " is already preloaded.\n\nDo you want to reload?" );
	skiploadedgeomids = !uiMSG().askGoOn( msg );
    }

    uiTaskRunner taskrunner( this );
    TypeSet<TrcKeyZSampling> tkzss;
    TypeSet<Pos::GeomID> loadgeomids;
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const Pos::GeomID& geomid = geomids[idx];
	if ( PLDM().isPresent(key,geomid) )
	{
	    if ( skiploadedgeomids ) continue;
	    PreLoader spl( key, geomid );
	    spl.unLoad();
	}

	dlg.getSampling( tkzs, geomid );
	tkzs.hsamp_.setLineRange( Interval<int>(geomid,geomid) );
	loadgeomids += geomid;
	tkzss += tkzs;
    }

    PreLoader spl( key, -1, &taskrunner );
    spl.load( tkzss, loadgeomids, dc.userType(), dlg.getScaler() );

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
    setCaption( tr("Pre-load %1").arg(uiStrings::sData().toLower()) );
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
    if ( selitms.isEmpty() )
	return;

    uiString msg = tr("Unload selected items?\n(This will not delete "
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


#define mDefaultNrTrcs 1000

// uiSeisPreLoadSel
uiSeisPreLoadSel::uiSeisPreLoadSel( uiParent* p, GeomType geom,
				    const MultiID& input )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),
				 mNoDlgTitle,
				 mODHelpKey(mSeisPreLoad2D3DDataHelpID))
	      .nrstatusflds(1))
    , scaler_(new LinScaler(0,1))
    , gen_(*new Stats::RandGen())
{
    setCaption( geom==Vol ? tr("Pre-load 3D Data") : tr("Pre-load 2D Data") );

    auto* leftgrp = new uiGroup( this, "Left Group" );
    IOObjContext ctxt = uiSeisSel::ioContext( geom, true );
    uiSeisSel::Setup sssu( geom );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    seissel_ = new uiSeisSel( leftgrp, ctxt, sssu );
    if ( !input.isUdf() )
	seissel_->setInput( input );

    mAttachCB( seissel_->selectionDone, uiSeisPreLoadSel::seisSel );

    SelSetup selsu( geom ); selsu.multiline(true);
    subselfld_ = uiSeisSubSel::get( leftgrp, selsu );
    mAttachCB( subselfld_->selChange, uiSeisPreLoadSel::selChangeCB );
    subselfld_->attach( alignedBelow, seissel_ );

    formatdiskfld_ = new uiGenInput( leftgrp, tr("Format on disk") );
    formatdiskfld_->setReadOnly();
    sizediskfld_ = new uiGenInput( leftgrp, tr("Size on disk") );
    sizediskfld_->setReadOnly();
    formatdiskfld_->attach( alignedBelow, subselfld_ );
    sizediskfld_->attach( rightTo, formatdiskfld_ );

    BufferStringSet formats;
    const BufferStringSet& keys = DataCharacteristics::UserTypeDef().keys();
    for ( int idx=0; idx<keys.size(); idx++ )
	formats.add( keys.get(idx).buf()+4 );

    typefld_ = new uiGenInput( leftgrp, tr("Load as"),
		StringListInpSpec(formats) );
    mAttachCB( typefld_->valuechanged, uiSeisPreLoadSel::selChangeCB );
    typefld_->attach( alignedBelow, formatdiskfld_ );

    memusagefld_ = new uiGenInput( leftgrp, tr("Est usage") );
    memusagefld_->setReadOnly();
    memusagefld_->attach( alignedBelow, sizediskfld_ );

    doscalefld_ = new uiGenInput( leftgrp, tr("Scale Values"),
				  BoolInpSpec(false) );
    mAttachCB( doscalefld_->valuechanged, uiSeisPreLoadSel::doScaleCB );
    doscalefld_->attach( alignedBelow, typefld_ );

    fromrgfld_ = new uiGenInput( leftgrp, tr("Scale From"),
	FloatInpIntervalSpec().setName("From start",0).setName("From stop",1) );
    fromrgfld_->attach( alignedBelow, doscalefld_ );

    torgfld_ = new uiGenInput( leftgrp, tr("Scale To"),
	FloatInpIntervalSpec().setName("To start",0).setName("To stop",1) );
    torgfld_->attach( alignedBelow, fromrgfld_ );

    auto* rightgrp = new uiGroup( this, "Right Group" );
    rightgrp->attach( rightOf, leftgrp );
    nrtrcsfld_ = new uiGenInput( rightgrp, tr("Nr Traces"),
				 IntInpSpec(mDefaultNrTrcs) );
    auto* scanbut = new uiPushButton( rightgrp, tr("Scan"), true );
    mAttachCB( scanbut->activated, uiSeisPreLoadSel::fillHist );
    scanbut->attach( rightTo, nrtrcsfld_ );
    histfld_ = new uiMapperRangeEditor( rightgrp, -1, false );
    mAttachCB( histfld_->rangeChanged, uiSeisPreLoadSel::histChangeCB );
    histfld_->attach( leftAlignedBelow, nrtrcsfld_ );

    mAttachCB( postFinalize(), uiSeisPreLoadSel::finalizeDoneCB );
}


uiSeisPreLoadSel::~uiSeisPreLoadSel()
{
    detachAllNotifiers();
    delete scaler_;
    delete &gen_;
}


void uiSeisPreLoadSel::finalizeDoneCB( CallBacker* )
{
    doScaleCB( nullptr );
    seisSel( nullptr );
}


void uiSeisPreLoadSel::doScaleCB( CallBacker* )
{
    const bool doscale = doscalefld_->getBoolValue();
    fromrgfld_->display( doscale );
    torgfld_->display( doscale );
}


const IOObj* uiSeisPreLoadSel::getIOObj() const
{ return seissel_->getIOObj(); }


void uiSeisPreLoadSel::getSampling( TrcKeyZSampling& tkzs ) const
{ subselfld_->getSampling( tkzs ); }


void uiSeisPreLoadSel::getSampling( TrcKeyZSampling& tkzs,
				    Pos::GeomID geomid ) const
{
    mDynamicCastGet(uiSeis2DSubSel*,ss2d,subselfld_)
    if ( !ss2d )
    {
	tkzs.setEmpty();
	return;
    }

    ss2d->getSampling( tkzs, geomid );
}


void uiSeisPreLoadSel::selectedGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    mDynamicCastGet(uiSeis2DSubSel*,ss2d,subselfld_)
    if ( ss2d )
	ss2d->selectedGeomIDs( geomids );
}


Scaler* uiSeisPreLoadSel::getScaler() const
{ return scaler_; }


void uiSeisPreLoadSel::fillHist( CallBacker* )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    SeisIOObjInfo info( ioobj );
    TrcKeyZSampling tkzs;
    if ( info.is2D() )
    {
	TypeSet<Pos::GeomID> geomids;
	selectedGeomIDs( geomids );
	if ( geomids.isEmpty() )
	{
	    uiMSG().error( tr("Please select at least 1 2D line") );
	    return;
	}

	Pos::GeomID geomid0 = geomids.first();
	tkzs.hsamp_.setLineRange( StepInterval<int>(geomid0,geomid0,1) );
	StepInterval<int> trcrg; StepInterval<float> zrg;
	if ( !info.getRanges(geomid0,trcrg,zrg) )
	    return;

	tkzs.hsamp_.setTrcRange( trcrg );
	tkzs.zsamp_ = zrg;
    }
    else
    {
	if ( !info.getRanges(tkzs) ) // TODO: Add message
	{
	    histfld_->setEmpty();
	    return;
	}
    }

    const od_int64 totalsz = tkzs.hsamp_.totalNr();
    const int nr2add = (int)mMIN(totalsz,nrtrcsfld_->getIntValue());

    TypeSet<od_int64> gidxs( nr2add, -1 );
    for ( int idx=0; idx<nr2add; idx++ )
    {
	const od_int64 vidx = gen_.getIndex( totalsz );
	gidxs[idx] = vidx;
    }

    sort( gidxs );
    SeisTrcBuf seisbuf( true );
    SeisTrcReader rdr( *ioobj );
    rdr.prepareWork();
    mDynamicCastGet(SeisTrcTranslator*,trl,rdr.translator())
    if ( !trl ) return;

    for ( int idx=0; idx<nr2add; idx++ )
    {
	const od_int64 gidx = gidxs[idx];
	bool res = true;
	if ( rdr.seisTranslator()->supportsGoTo() )
	    res = rdr.seisTranslator()->goTo( tkzs.hsamp_.atIndex(gidx) );

	if ( !res ) continue;

	auto* trc = new SeisTrc;
	res = rdr.get( *trc );
	if ( !res ) continue;

	seisbuf.add( trc );
    }

    SeisTrcBufArray2D array( &seisbuf, false, 0 );
    histfld_->setData( &array );

    ColTab::Sequence seq( "" );
    ColTab::MapperSetup ms; ms.range_ = histfld_->getDisplay().xAxis()->range();
    IOPar pars;
    if ( info.getDisplayPars(pars) )
    {
	const char* seqnm = pars.find( sKey::Name() );
	seq = ColTab::Sequence( seqnm );
	ms.usePar( pars );
    }

    histfld_->setColTabSeq( seq );
    histfld_->setColTabMapperSetup( ms );
    histChangeCB( 0 );
}


void uiSeisPreLoadSel::histChangeCB( CallBacker* )
{
    const Interval<float> rg = histfld_->getColTabMapperSetup().range_;
    fromrgfld_->setValue( rg );
}


void uiSeisPreLoadSel::seisSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    NotifyStopper ns( subselfld_->selChange );
    const SeisIOObjInfo info( ioobj );
    typefld_->setValue( 0 );
    subselfld_->setInput( *ioobj );

    BufferString formatstr;
    DataCharacteristics dc; info.getDataChar( dc );
    const FixedString usertypestr =
	DataCharacteristics::toString( dc.userType() );
    if ( usertypestr.size() > 4 )
	formatstr.set( usertypestr.buf()+4 );
    formatdiskfld_->setText( formatstr );

    const od_int64 filesz = info.getFileSize();
    sizediskfld_->setText( File::getFileSizeString(filesz) );

    selChangeCB( 0 );
}


void uiSeisPreLoadSel::selChangeCB( CallBacker* )
{
    updateScaleFld();
    updateEstUsage();
}


void uiSeisPreLoadSel::getDataChar( DataCharacteristics& dc ) const
{
    const DataCharacteristics::UserType type(
		(DataCharacteristics::UserType)typefld_->getIntValue() );
    if ( type == DataCharacteristics::Auto )
    {
	SeisIOObjInfo info( seissel_->ioobj(true) );
	info.getDataChar( dc );
    }
    else
	dc = DataCharacteristics( type );
}


void uiSeisPreLoadSel::updateScaleFld()
{
    SeisIOObjInfo info( seissel_->ioobj() );
    DataCharacteristics dcstor; info.getDataChar( dcstor );
    DataCharacteristics dc; getDataChar( dc );
    const bool doscale = dc.nrBytes()<dcstor.nrBytes() ||
			 dc.isSigned()!=dcstor.isSigned();
    if ( doscale )
	torgfld_->setValue( Interval<double>(dc.getLimitValue(false),
					     dc.getLimitValue(true)) );
    doscalefld_->setValue( doscale );
    doscalefld_->setReadOnly( !doscale );
    doScaleCB( 0 );
}


void uiSeisPreLoadSel::updateEstUsage()
{
    SeisIOObjInfo info( seissel_->ioobj() );
    const int nrcomp = info.nrComponents();

    BufferString infotxt;
    if ( nrcomp > 0 )
    {
	DataCharacteristics dc;
	getDataChar( dc );
	const od_int64 nrs = subselfld_->expectedNrSamples();
	const od_int64 nrt = subselfld_->expectedNrTraces();
	const od_int64 nrbytes = nrcomp * nrs * nrt * dc.nrBytes();
	infotxt.set( File::getFileSizeString(nrbytes/1024) );
    }
    else
	infotxt.set( "-" );

    memusagefld_->setText( infotxt );
}


bool uiSeisPreLoadSel::acceptOK( CallBacker* )
{
    mDynamicCastGet(LinScaler*,linscaler,scaler_)
    linscaler->constant = 0; linscaler->factor = 1;
    if ( doscalefld_->getBoolValue() )
    {
	linscaler->set( fromrgfld_->getDValue(0), torgfld_->getDValue(0),
			fromrgfld_->getDValue(1), torgfld_->getDValue(1) );
    }

    return seissel_->ioobj();
}



// uiSeisPreLoadedDataSel
uiSeisPreLoadedDataSel::uiSeisPreLoadedDataSel( uiParent* p, GeomType geom,
						const uiString& txt )
    : uiGroup(p,"Pre-loaded Data Selection")
    , geomtype_(geom)
    , selkey_(MultiID::udf())
    , selectionChanged(this)
{
    auto* lcb = new uiLabeledComboBox( this, txt );
    nmfld_ = lcb->box();
    nmfld_->setHSzPol( uiObject::Wide );
    mAttachCB( nmfld_->selectionChanged, uiSeisPreLoadedDataSel::selCB );

    selbut_ = uiButton::getStd( this, OD::Select,
			mCB(this,uiSeisPreLoadedDataSel,selPushCB), false );
    selbut_->attach( rightOf, lcb );

    preloadbut_ = new uiPushButton( this, tr("Pre-load"),
			mCB(this,uiSeisPreLoadedDataSel,preloadCB), false );
    preloadbut_->attach( rightOf, selbut_ );

    mAttachCB( PLDM().changed, uiSeisPreLoadedDataSel::updateCB );
    setHAlignObj( lcb );
    updateCB( nullptr );
}


uiSeisPreLoadedDataSel::~uiSeisPreLoadedDataSel()
{
    detachAllNotifiers();
}


void uiSeisPreLoadedDataSel::setInput( const MultiID& inpkey, int compnr )
{
    const int selidx = keys_.indexOf( inpkey );
    if ( selidx < 0 )
	return;


    ConstRefMan<SeisDataPack> seisdp = PLDM().get<SeisDataPack>(inpkey );
    if ( !seisdp )
	return;

    nmfld_->setCurrentItem( selidx+1 );
    selkey_ = inpkey;
    compnr_ = (compnr >= 0 && compnr < seisdp->nrComponents()) ? compnr : 0;
}


const MultiID& uiSeisPreLoadedDataSel::selectedKey() const
{
    return selkey_;
}


const char* uiSeisPreLoadedDataSel::selectedName() const
{
    return nmfld_->text();
}


int uiSeisPreLoadedDataSel::selectedCompNr() const
{
    return compnr_;
}


const char* uiSeisPreLoadedDataSel::selectedCompName() const
{
    if ( selkey_.isUdf() )
	return nullptr;

    ConstRefMan<SeisDataPack> dp = PLDM().get<SeisDataPack>(selkey_ );
    if ( !dp || dp->nrComponents()==1 || compnr_ < dp->nrComponents() )
	return nullptr;

    return dp->getComponentName( compnr_ );
}


DataPack::ID uiSeisPreLoadedDataSel::selectedDPID() const
{
    ConstRefMan<DataPack> dp = PLDM().getDP( selkey_ );
    return dp ? dp->id() : DataPack::ID::udf();
}


void uiSeisPreLoadedDataSel::updateCB( CallBacker* )
{
    keys_.erase();
    names_.setEmpty();
    const ObjectSet<PreLoadDataEntry>& plentries = PLDM().getEntries();
    for ( int idx=0; idx<plentries.size(); idx++ )
    {
	const SeisIOObjInfo objinfo( plentries[idx]->mid_ );
	if ( objinfo.geomType() != geomtype_ )
	    continue;

	keys_.add( plentries[idx]->mid_ );
	names_.add( plentries[idx]->name_ );
    }

    const BufferString curselnm = nmfld_->text();
    nmfld_->setEmpty();
    nmfld_->addItem( OD::String::empty() );
    nmfld_->addItems( names_ );
    if ( !curselnm.isEmpty() && names_.isPresent(curselnm) )
	nmfld_->setCurrentItem( curselnm );
    else
    {
	selkey_.setUdf();
	nmfld_->setCurrentItem( 0 );
	compnr_ = 0;
    }
}


void uiSeisPreLoadedDataSel::selCB( CallBacker* )
{
    const int selidx = nmfld_->currentItem() - 1;
    if ( selidx < 0 )
    {
	selkey_ = MultiID::udf();
	return;
    }

    const MultiID selkey = keys_[selidx];
    ConstRefMan<SeisDataPack> dp = PLDM().get<SeisDataPack>(selkey );
    if ( !dp )
	return;

    if ( dp->nrComponents() == 1 )
	compnr_ = 0;
    else // multi-comp
    {
	BufferStringSet compnms;
	for ( int idx=0; idx<dp->nrComponents(); idx++ )
	    compnms.add( dp->getComponentName(idx) );

	uiSelectFromList::Setup su(
		uiStrings::phrSelect(uiStrings::sComponent()), compnms );
	uiSelectFromList seldlg( this, su );
	if ( !seldlg.go() || seldlg.selection() < 0 )
	{
	    setInput( selkey_, compnr_ );
	    return;
	}

	compnr_ = seldlg.selection();
    }

    selkey_ = selkey;
    selectionChanged.trigger();
}


class uiSeisPLDataSelDlg : public uiSelectFromList
{
public:
uiSeisPLDataSelDlg( uiParent* p, const uiSelectFromList::Setup& su,
		    const TypeSet<MultiID>& keys )
    : uiSelectFromList(p,su)
    , keys_(keys)
{
    compfld_ = new uiLabeledComboBox( this, uiStrings::sComponent() );
    compfld_->box()->setHSzPol( uiObject::Wide );
    compfld_->attach( alignedBelow, selFld() );
    compfld_->display( false );

    mAttachCB( selFld()->selectionChanged, uiSeisPLDataSelDlg::selCB );
}


~uiSeisPLDataSelDlg()
{
    detachAllNotifiers();
}


void selCB( CallBacker* )
{
    compfld_->box()->setEmpty();
    const int selidx = selFld()->currentItem();
    if ( !keys_.validIdx(selidx) )
	return;

    ConstRefMan<SeisDataPack> seisdp = PLDM().get<SeisDataPack>( keys_[selidx]);
    if ( !seisdp )
	return;

    const int nrcomps = seisdp->nrComponents();
    if ( nrcomps < 2 )
    {
	compfld_->display( false );
	return;
    }

    for ( int idx=0; idx<nrcomps; idx++ )
	compfld_->box()->addItem( seisdp->getComponentName(idx) );

    compfld_->display( true );
}

    uiLabeledComboBox*		compfld_;
    const TypeSet<MultiID>&	keys_;
};


void uiSeisPreLoadedDataSel::selPushCB( CallBacker* )
{
    if ( keys_.isEmpty() )
    {
	uiString msg( tr("No pre-loaded volumes are available at the moment.\n"
			 "Do you want to pre-load one now?") );
	if ( uiMSG().askGoOn(msg,tr("Pre-load"),uiStrings::sCancel()) )
	    preloadCB( nullptr );

	return;
    }

    uiSelectFromList::Setup su( uiStrings::sSelect(), names_ );
    uiSeisPLDataSelDlg seldlg( this, su, keys_ );
    if ( !seldlg.go() || seldlg.selection() < 0 )
	return;

    NotifyStopper ns( nmfld_->selectionChanged );
    nmfld_->setCurrentItem( seldlg.selection() + 1 );
    selkey_ = keys_.get( seldlg.selection() );
    compnr_ = seldlg.compfld_->box()->currentItem();
    if ( compnr_ < 0 )
	compnr_ = 0;

    selectionChanged.trigger();
}


void uiSeisPreLoadedDataSel::preloadCB( CallBacker* )
{
    bool doselect = keys_.isEmpty();
    NotifyStopper ns( PLDM().changed );
    uiSeisPreLoadMgr pldlg( this );
    pldlg.go();

    updateCB( nullptr );
    if ( doselect && !keys_.isEmpty() )
    {
	setInput( keys_.first() );
	selCB( nullptr );
    }
}
