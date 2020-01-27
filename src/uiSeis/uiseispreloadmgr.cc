/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2009
-*/


#include "uiseispreloadmgr.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "cubesubsel.h"
#include "datapack.h"
#include "file.h"
#include "ioobj.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "preloads.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "seisselsetup.h"
#include "seispreload.h"
#include "seispsioprov.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "statrand.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uiaxishandler.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimapperrangeeditor.h"
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
	mAddBut(tr("Load Cube"),cubeLoadPush,"seismiccube")
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut(tr("Load 2D DataSet"),linesLoadPush,"seismicline2d")
	else
	    mAddBut(tr("Load DataSet"),linesLoadPush,"seismicline2d")
    }
    if ( has3d )
    {
	if ( has2d )
	    mAddBut(tr("Add 3D Prestack data"),ps3DPush,"prestackdataset")
	else
	    mAddBut(tr("Add Prestack data"),ps3DPush,"prestackdataset")
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut(tr("Add 2D Prestack lines"),ps2DPush,
		    "prestackdataset2d")
	else
	    mAddBut(tr("Add Prestack data"),ps2DPush,"prestackdataset2d")
    }
    mAddBut(tr("Unload"),unloadPush,"unload");

    uiToolButton* savetb = new uiToolButton( listfld_, "save",
	    tr("Save pre-loads"), mCB(this,uiSeisPreLoadMgr,savePush) );
    savetb->attach( rightAlignedAbove, listfld_->box() );
    uiToolButton* opentb = new uiToolButton( listfld_, "open",
	    tr("Retrieve pre-loads"), mCB(this,uiSeisPreLoadMgr,openPush) );
    opentb->attach( leftOf, savetb );

    uiGroup* infogrp = new uiGroup( this, "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info" );
    infofld_->setPrefHeightInChar( 8 );

    uiSplitter* spl = new uiSplitter( this, "Splitter", OD::Horizontal );
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

    const DBKey mid = entries[selidx]->dbkey_;
    SeisIOObjInfo ioinf( mid );
    if ( !ioinf.isOK() )
	{ infofld_->setText(toUiString("Internal: IOObj not OK")); return; }

    BufferString disptxt;
    PLDM().getInfo( mid, entries[selidx]->geomid_, disptxt );
    infofld_->setText( disptxt );
}


#define mCheckIOObjExistance( ioobj ) \
if ( !ioobj->implExists( true ) ) \
{ \
    uiString msg = toUiString("%1 '%2'").arg(toUiString(cannotloadstr)). \
		   arg(ioobj->name()); \
    uiMSG().error( msg ); \
    return; \
}

void uiSeisPreLoadMgr::cubeLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Vol, initmid_ );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );
    const DBKey key = ioobj->key();
    if ( PLDM().isPresent(key) )
    {
	uiString msg = tr("'%1' is already preloaded."
			"\n\nDo you want to reload the cube?")
			.arg( ioobj->name() );
	if ( !uiMSG().askGoOn(msg) ) return;

	PreLoader spl( key );
	spl.unLoad();
    }

    TrcKeyZSampling tkzs; dlg.getSampling( tkzs );
    DataCharacteristics dc; dlg.getDataChar( dc );
    PreLoader spl( key );
    uiTaskRunnerProvider trprov( this );
    spl.setTaskRunner( trprov );
    const CubeSubSel css( tkzs );
    if ( !spl.load(&css,dc.userType(),dlg.getScaler()) )
    {
	const uiString emsg = spl.errMsg();
	if ( !emsg.isEmpty() )
	    uiMSG().error( emsg );
    }

    fullUpd(0);
}


void uiSeisPreLoadMgr::linesLoadPush( CallBacker* )
{
    uiSeisPreLoadSel dlg( this, Line, initmid_ );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );

    TrcKeyZSampling tkzs;
    tkzs.hsamp_.setIs2D();
    DataCharacteristics dc; dlg.getDataChar( dc );
    GeomIDSet geomids;
    dlg.selectedGeomIDs( geomids );
    const DBKey key = ioobj->key();
    GeomIDSet loadedgeomids;
    for ( int idx=0; idx<geomids.size(); idx++ )
	if ( PLDM().isPresent(key,geomids[idx]) )
	    loadedgeomids += geomids[idx];

    bool skiploadedgeomids = false;
    if ( !loadedgeomids.isEmpty() )
    {
	BufferStringSet lnms;
	for ( int idx=0; idx<loadedgeomids.size(); idx++ )
	    lnms.add( loadedgeomids[idx].name() );
	uiString msg = tr("%1 dataset for lines %2 is already preloaded")
			    .arg( key.name() )
			    .arg( lnms.getDispString() );
	msg.appendPhrase( tr("Do you want to reload?"),
			  uiString::NoSep, uiString::AfterEmptyLine );
	skiploadedgeomids = !uiMSG().askGoOn( msg );
    }
    uiTaskRunnerProvider trprov( this );
    TypeSet<TrcKeyZSampling> tkzss;
    GeomIDSet loadgeomids;
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const Pos::GeomID& geomid = geomids[idx];
	if ( PLDM().isPresent(key,geomid) )
	{
	    if ( skiploadedgeomids )
		continue;
	    PreLoader spl( key );
	    spl.unLoad( geomid );
	}

	dlg.getSampling( tkzs, geomid );
	const auto lnr = geomid.lineNr();
	tkzs.hsamp_.setLineRange( Interval<int>(lnr,lnr) );
	loadgeomids += geomid;
	tkzss += tkzs;
    }

    PreLoader spl( key, trprov );
    ObjectSet<Survey::GeomSubSel> gsss;
    for ( auto tkzsamp : tkzss )
	if ( tkzsamp.is2D() )
	    gsss += new LineSubSel( tkzsamp );
	else
	    gsss += new CubeSubSel( tkzsamp );
    spl.load( gsss, dc.userType(), dlg.getScaler() );
    deepErase( gsss );

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
    uiTaskRunnerProvider trprov( this );
    spl.setTaskRunner( trprov );
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

bool acceptOK()
{
    if ( !ioObj() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("a 2D Prestack Data Store")) );
	return false;
    }
    lnms_.erase();
    lnmsfld_->getChosen( lnms_ );
    if ( lnms_.isEmpty() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("one or more lines")) );
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
    uiTaskRunnerProvider trprov( this );
    spl.setTaskRunner( trprov );
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
	PreLoader spl( entries[selidx]->dbkey_ );
	spl.unLoad( entries[selidx]->geomid_ );
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

    const BufferString fnm( dlg.ioObj()->mainFileName() );
    delete ctio.ioobj_;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open input file:\n%1").arg(fnm) )

    ascistream astrm( strm,true );
    IOPar iop( astrm );
    if ( iop.isEmpty() )
	mErrRet( tr("No valid objects found") )

    uiTaskRunnerProvider trprov( this );
    PreLoader::load( iop, trprov );
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

    const BufferString fnm( dlg.ioObj()->mainFileName() );
    delete ctio.ioobj_;
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open output file:\n%1").arg(fnm) )

    IOPar alliop;
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	IOPar iop;
	PreLoader spl( entries[idx]->dbkey_ );
	spl.setDefGeomID( entries[idx]->geomid_ );
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
				    const DBKey& input )
    : uiDialog(p,uiDialog::Setup(uiString::empty(),
				 mNoDlgTitle,
				 mODHelpKey(mSeisPreLoad2D3DDataHelpID)))
    , scaler_(new LinScaler(0,1))
{
    setCaption( geom==Vol ? tr("Pre-load 3D Data") : tr("Pre-load 2D Data") );

    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    IOObjContext ctxt = uiSeisSel::ioContext( geom, true );
    uiSeisSel::Setup sssu( geom );
    sssu.steerpol( Seis::InclSteer );
    seissel_ = new uiSeisSel( leftgrp, ctxt, sssu );
    if ( !input.isValid() )
	seissel_->setInput( input );
    seissel_->selectionDone.notify( mCB(this,uiSeisPreLoadSel,seisSel) );

    SelSetup selsu( geom ); selsu.multiline(true);
    subselfld_ = uiSeisSubSel::get( leftgrp, selsu );
    subselfld_->selChange.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
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
    typefld_->valuechanged.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
    typefld_->attach( alignedBelow, formatdiskfld_ );

    memusagefld_ = new uiGenInput( leftgrp, tr("Est usage") );
    memusagefld_->setReadOnly();
    memusagefld_->attach( alignedBelow, sizediskfld_ );

    doscalefld_ = new uiGenInput( leftgrp, tr("Scale Values"),
				  BoolInpSpec(false) );
    doscalefld_->valuechanged.notify( mCB(this,uiSeisPreLoadSel,doScaleCB) );
    doscalefld_->attach( alignedBelow, typefld_ );

    fromrgfld_ = new uiGenInput( leftgrp, tr("Scale From"),
	FloatInpIntervalSpec().setName("From start",0).setName("From stop",1) );
    fromrgfld_->attach( alignedBelow, doscalefld_ );

    torgfld_ = new uiGenInput( leftgrp, tr("Scale To"),
	FloatInpIntervalSpec().setName("To start",0).setName("To stop",1) );
    torgfld_->attach( alignedBelow, fromrgfld_ );

    uiGroup* rightgrp = new uiGroup( this, "Right Group" );
    rightgrp->attach( rightOf, leftgrp );
    nrtrcsfld_ = new uiGenInput( rightgrp, tr("Nr Traces"),
				 IntInpSpec(mDefaultNrTrcs) );
    scanbut_ = new uiPushButton( rightgrp, tr("Rescan"), true );
    scanbut_->activated.notify( mCB(this,uiSeisPreLoadSel,fillHist) );
    scanbut_->attach( rightTo, nrtrcsfld_ );
    mapperrgfld_ = new uiMapperRangeEditor( rightgrp, false );
    mapperrgfld_->attach( alignedBelow, nrtrcsfld_ );
    mAttachCB( mapperrgfld_->mapper().setup().objectChanged(),
		uiSeisPreLoadSel::mapperSetupChgCB );

    postFinalise().notify( mCB(this,uiSeisPreLoadSel,finalizeDoneCB) );
}


uiSeisPreLoadSel::~uiSeisPreLoadSel()
{
    delete scaler_;
}


void uiSeisPreLoadSel::finalizeDoneCB( CallBacker* )
{
    typefld_->setValue( (int)OD::AutoDataRep );
    doScaleCB( 0 );
    seisSel( 0 );
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
	{ tkzs.set2DDef(); return; }

    ss2d->getTKZS( tkzs, geomid );
}


void uiSeisPreLoadSel::selectedGeomIDs( GeomIDSet& geomids ) const
{
    mDynamicCastGet(uiSeis2DSubSel*,ss2d,subselfld_)
    if ( ss2d )
	ss2d->selectedGeomIDs( geomids );
}


const Scaler* uiSeisPreLoadSel::getScaler() const
{ return scaler_; }


void uiSeisPreLoadSel::fillHist( CallBacker* )
{
    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    const SeisIOObjInfo info( ioobj );
    TrcKeyZSampling tkzs;
    const bool is2d = info.is2D();
    if ( is2d )
    {
	tkzs.hsamp_.setIs2D();
	GeomIDSet geomids;
	selectedGeomIDs( geomids );
	if ( geomids.isEmpty() )
	    return;

	Pos::GeomID geomid0 = geomids.first();
	const auto lnr = geomid0.lineNr();
	tkzs.hsamp_.setLineRange( StepInterval<int>(lnr,lnr,1) );
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
	    mapperrgfld_->setEmpty();
	    return;
	}
    }

    IOPar iop;
    if ( info.getStats(iop) && mapperrgfld_->setData(iop) )
    {
	nrtrcsfld_->setValue( mCast(int,tkzs.hsamp_.totalNr()) );
	setColorTable();
	return;
    }

    const od_int64 totalsz = tkzs.hsamp_.totalNr();
    const int nr2add = (int)mMIN(totalsz,nrtrcsfld_->getIntValue());

    TypeSet<od_int64> gidxs( nr2add, -1 );
    od_int64 randint = Stats::randGen().getIndex( mUdf(int) );
    for ( int idx=0; idx<nr2add; idx++ )
    {
	od_int64 vidx = Stats::randGen().getIndexFast( totalsz, randint );
	gidxs[idx] = vidx;
	randint *= mCast( int, vidx );
    }

    sort( gidxs );
    SeisTrcBuf seisbuf( true );
    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj, &uirv );
    if ( !prov )
	mErrRet( uirv );

    for ( int idx=0; idx<nr2add; idx++ )
    {
	const od_int64 gidx = gidxs[idx];
	const TrcKey trckey = TrcKey( tkzs.hsamp_.atIndex(gidx) );
	if ( !prov->isPresent(trckey) )
	    continue;

	SeisTrc* trc = new SeisTrc;
	const uiRetVal retval = prov->getAt( trckey, *trc );
	if ( !retval.isOK() )
	    { delete trc; continue; }

	seisbuf.add( trc );
    }

    const SeisTrcBufArray2D array( &seisbuf, false, 0 );
    mapperrgfld_->setData( &array );
    setColorTable();
}


void uiSeisPreLoadSel::setColorTable()
{
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    const SeisIOObjInfo info( ioobj );
    ConstRefMan<ColTab::Sequence> seq = ColTab::SeqMGR().getDefault();
    RefMan<ColTab::Mapper> mpr = new ColTab::Mapper;
    mpr->setup().setFixedRange( mapperrgfld_->getDisplay().xAxis()->range() );
    IOPar pars;
    if ( info.getDisplayPars(pars) )
    {
	const char* seqnm = pars.find( sKey::Name() );
	seq = ColTab::SeqMGR().getAny( seqnm );
	mpr->setup().usePar( pars );
    }

    mapperrgfld_->setColTabSeq( *seq );
    mapperrgfld_->setMapper( *mpr );
}


void uiSeisPreLoadSel::mapperSetupChgCB( CallBacker* )
{
    fromrgfld_->setValue( mapperrgfld_->mapper().getRange() );
}


void uiSeisPreLoadSel::seisSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj ) return;

    NotifyStopper ns( subselfld_->selChange );
    const SeisIOObjInfo info( ioobj ); IOPar iop;
    const bool dorescan = !info.getStats( iop );
    nrtrcsfld_->setReadOnly( !dorescan );
    nrtrcsfld_->setValue( mDefaultNrTrcs );
    scanbut_->setSensitive( dorescan );
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
    fillHist( 0 );
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
    if ( type == OD::AutoDataRep )
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


bool uiSeisPreLoadSel::acceptOK()
{
    mDynamicCastGet(LinScaler*,linscaler,scaler_)
    linscaler->constant_ = 0; linscaler->factor_ = 1;
    if ( doscalefld_->getBoolValue() )
    {
	linscaler->set( fromrgfld_->getDValue(0), torgfld_->getDValue(0),
			fromrgfld_->getDValue(1), torgfld_->getDValue(1) );
    }

    return seissel_->ioobj();
}
