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
    listfld_ = new uiListBox( topgrp, "Loaded entries", OD::ChooseZeroOrMore );
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
	mAddBut(tr("Add Cube"),cubeLoadPush,"seismiccube")
    }
    if ( has2d )
    {
	if ( has3d )
	    mAddBut(tr("Add 2D DataSet"),linesLoadPush,"seismicline2d")
	else
	    mAddBut(tr("Add DataSet"),linesLoadPush,"seismicline2d")
    }
    mAddBut(tr("Unload Checked"),unloadPush,"unload");

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
	listfld_->addItem( toUiString(entries[idx]->name_) );

    listfld_->setCurrentItem( 0 );
}


void uiSeisPreLoadMgr::selChg( CallBacker* )
{
    const int selidx = listfld_->currentItem();
    const ObjectSet<PreLoadDataEntry>& entries = PLDM().getEntries();
    if ( !entries.validIdx(selidx) )
	{ infofld_->setText(""); return; }

    const DBKey mid = entries[selidx]->dbkey_;
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
    uiSeisPreLoadSel dlg( this, Vol, initmid_ );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );
    const DBKey key = ioobj->key();
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
    uiSeisPreLoadSel dlg( this, Line, initmid_ );
    if ( !dlg.go() ) return;

    const IOObj* ioobj = dlg.getIOObj();
    mCheckIOObjExistance( ioobj );

    TrcKeyZSampling tkzs;
    DataCharacteristics dc; dlg.getDataChar( dc );
    TypeSet<Pos::GeomID> geomids;
    dlg.selectedGeomIDs( geomids );
    const DBKey key = ioobj->key();
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
	PreLoader spl( entries[selidx]->dbkey_, entries[selidx]->geomid_ );
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
	PreLoader spl( entries[idx]->dbkey_, entries[idx]->geomid_ );
	spl.fillPar( iop );
	alliop.mergeComp( iop, IOPar::compKey("Seis",idx) );
    }

    ascostream astrm( strm );
    if ( !astrm.putHeader("Pre-loads") )
	mErrRet( tr("Cannot write to output file:\n%1").arg(fnm) )
    alliop.putTo( astrm );
}


// uiSeisPreLoadSel
uiSeisPreLoadSel::uiSeisPreLoadSel( uiParent* p, GeomType geom,
				    const DBKey& input )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),
					mNoDlgTitle,mNoHelpKey).nrstatusflds(1))
    , scaler_(new LinScaler(0,1))
{
    setCaption( geom==Vol ? tr("Pre-load 3D Data") : tr("Pre-load 2D Data") );

    uiGroup* leftgrp = new uiGroup( this, "Left Group" );
    IOObjContext ctxt = uiSeisSel::ioContext( geom, true );
    uiSeisSel::Setup sssu( geom );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );
    seissel_ = new uiSeisSel( leftgrp, ctxt, sssu );
    if ( !input.isValid() )
	seissel_->setInput( input );
    seissel_->selectionDone.notify( mCB(this,uiSeisPreLoadSel,seisSel) );

    SelSetup selsu( geom ); selsu.multiline(true);
    subselfld_ = uiSeisSubSel::get( leftgrp, selsu );
    subselfld_->selChange.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
    subselfld_->attach( alignedBelow, seissel_ );

    typefld_ = new uiGenInput( leftgrp, tr("Load as"),
		StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    typefld_->valuechanged.notify( mCB(this,uiSeisPreLoadSel,selChangeCB) );
    typefld_->setValue( (int)DataCharacteristics::Auto );
    typefld_->attach( alignedBelow, subselfld_ );

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
    nrtrcsfld_ = new uiGenInput( rightgrp, tr("Nr Traces"), IntInpSpec(1000) );
    uiPushButton* scanbut = new uiPushButton( rightgrp, tr("Scan"), true );
    scanbut->activated.notify( mCB(this,uiSeisPreLoadSel,fillHist) );
    scanbut->attach( rightTo, nrtrcsfld_ );
    histfld_ = new uiMapperRangeEditor( rightgrp, -1, false );
    histfld_->rangeChanged.notify( mCB(this,uiSeisPreLoadSel,histChangeCB) );
    histfld_->attach( leftAlignedBelow, nrtrcsfld_ );

    postFinalise().notify( mCB(this,uiSeisPreLoadSel,finalizeDoneCB) );
}


uiSeisPreLoadSel::~uiSeisPreLoadSel()
{
    delete scaler_;
}


void uiSeisPreLoadSel::finalizeDoneCB( CallBacker* )
{
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
    SeisTrcReader rdr( ioobj );
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

	SeisTrc* trc = new SeisTrc;
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

    typefld_->setValue( 0 );
    subselfld_->setInput( *ioobj );
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


#define mGetExtremeVal( rg, positiveextreme ) \
    ((samesign && positiveextreme^(rg.start>0)) ? 0 : \
    (positiveextreme ? mMAX(rg.start,rg.stop) : mMIN(rg.start,rg.stop)));

void uiSeisPreLoadSel::updateScaleFld()
{
    SeisIOObjInfo info( seissel_->ioobj() );
    DataCharacteristics dcstor; info.getDataChar( dcstor );
    DataCharacteristics dc; getDataChar( dc );
    Interval<double> intv; intv.setUdf();
    if ( dc.nrBytes() < dcstor.nrBytes() || dc.isSigned() != dcstor.isSigned() )
    {
	intv.start = dc.getLimitValue(false);
	intv.stop = dc.getLimitValue(true);
    }

    torgfld_->setValue( intv );

    const DataCharacteristics::UserType type(
		(DataCharacteristics::UserType)typefld_->getIntValue() );
    doscalefld_->setValue( type != DataCharacteristics::Auto );
    doScaleCB( 0 );
}


void uiSeisPreLoadSel::updateEstUsage()
{
    SeisIOObjInfo info( seissel_->ioobj() );
    const int nrcomp = info.nrComponents();

    uiString infotxt = uiStrings::phrData(tr("format on disk: "));
    if ( nrcomp > 0 )
    {
	DataCharacteristics dc; info.getDataChar( dc );
	const FixedString usertypestr =
	    DataCharacteristics::toString( dc.userType() );
	if ( usertypestr.size() > 4 )
	    infotxt.append( usertypestr.buf()+4 );

	getDataChar( dc );
	const od_int64 nrs = subselfld_->expectedNrSamples();
	const od_int64 nrt = subselfld_->expectedNrTraces();
	const od_int64 nrbytes = nrcomp * nrs * nrt * dc.nrBytes();
	infotxt.append( tr(". Estimated memory usage: ") )
	       .append( File::getFileSizeString(nrbytes/1024) );
    }
    else
	infotxt.append( "?" );

    toStatusBar( infotxt );
}


bool uiSeisPreLoadSel::acceptOK()
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
