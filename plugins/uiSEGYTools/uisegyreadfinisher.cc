/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyreadfinisher.h"

#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uitaskrunner.h"
#include "uiwellsel.h"
#include "ui2dgeomman.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segyscanner.h"
#include "segytr.h"
#include "seiscbvs.h"
#include "seisimporter.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survgeom2d.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"
#include "wellwriter.h"

#include "coordsystem.h"

#define mUdfGeomID Survey::GeometryManager::cUndefGeomID()

uiString uiSEGYReadFinisher::getWinTile( const FullSpec& fs )
{
    const Seis::GeomType gt = fs.geomType();
    const bool isvsp = fs.isVSP();

    uiString ret;
    if ( fs.spec_.nrFiles() > 1 && !isvsp && Seis::is2D(gt) )
	ret = tr("Import %1s");
    else
	ret = tr("Import %1");

    if ( isvsp )
	ret.arg( tr("Zero-offset VSP") );
    else
	ret.arg( Seis::nameOf(gt) );
    return ret;
}


uiString uiSEGYReadFinisher::getDlgTitle( const char* usrspec )
{
    return tr("Importing %1").arg( usrspec );
}


uiSEGYReadFinisher::uiSEGYReadFinisher( uiParent* p, const FullSpec& fs,
					const char* usrspec )
    : uiDialog(p,uiDialog::Setup(getWinTile(fs),getDlgTitle(usrspec),
				 mODHelpKey(mSEGYReadFinisherHelpID)))
    , fs_(fs)
    , outwllfld_(nullptr)
    , lognmfld_(nullptr)
    , inpdomfld_(nullptr)
    , isfeetfld_(nullptr)
    , outimpfld_(nullptr)
    , outscanfld_(nullptr)
    , transffld_(nullptr)
    , remnullfld_(nullptr)
    , lnmfld_(nullptr)
    , docopyfld_(nullptr)
    , coordsfromfld_(nullptr)
    , coordfileextfld_(nullptr)
    , coordfilefld_(nullptr)
    , batchfld_(nullptr)
{
    setOkText( uiStrings::sImport() );
    objname_ = FilePath( usrspec ).baseName();
    const bool is2d = Seis::is2D( fs_.geomType() );
    if ( !is2d )
	objname_.replace( '*', 'x' );

    if ( fs_.isVSP() )
	crVSPFields();
    else
	crSeisFields();

    postFinalize().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


void uiSEGYReadFinisher::crSeisFields()
{
    const Seis::GeomType gt = fs_.geomType();
    const bool is2d = Seis::is2D( gt );
    const bool ismulti = fs_.spec_.nrFiles() > 1;

    docopyfld_ = new uiGenInput( this, tr("Import as"),
		BoolInpSpec(true,tr("OpendTect CBVS (copy&&import)"),
				 tr("SEGYDirect (scan&&link)")) );
    docopyfld_->valueChanged.notify( mCB(this,uiSEGYReadFinisher,doScanChg) );

    uiSeisTransfer::Setup trsu( gt );
    trsu.withnullfill( false ).fornewentry( true );
    transffld_ = new uiSeisTransfer( this, trsu );
    transffld_->attach( alignedBelow, docopyfld_ );

    remnullfld_ = new uiGenInput( this, tr("Null traces"),
		BoolInpSpec(true,uiStrings::sDiscard(),uiStrings::sPass()) );

    if ( is2d )
    {
	lnmfld_ = new uiSeis2DLineNameSel( this, false );
	lnmfld_->attach( alignedBelow, docopyfld_ );
	remnullfld_->attach( alignedBelow, lnmfld_ );

	uiSeis2DSubSel& selfld = *transffld_->selFld2D();
	if ( !ismulti )
	{
	    selfld.setSelectedLine( objname_.buf() );
	    lnmfld_->setInput( objname_.buf() );
	}
	else
	{
	    selfld.setSelectedLine( "*" );
	    selfld.setSensitive( false );
	    lnmfld_->setInput( "*" );
	    lnmfld_->setSensitive( false );
	}
    }
    else
	remnullfld_->attach( alignedBelow, docopyfld_ );

    uiGroup* attgrp = transffld_;
    if ( is2d )
	cr2DCoordSrcFields( attgrp, ismulti );

    const BufferStringSet trnotallowed( "SEGYDirect" );
    uiSeisSel::Setup copysu( gt );
    copysu.enabotherdomain( true ).withwriteopts(true)
	  .trsnotallwed(trnotallowed);
    IOObjContext ctxt( uiSeisSel::ioContext( gt, false ) );
    outimpfld_ = new uiSeisSel( this, ctxt, copysu );
    outimpfld_->attach( alignedBelow, attgrp );
    if ( !is2d )
	outimpfld_->setInputText( objname_ );

    uiSeisSel::Setup scansu( gt );
    scansu.enabotherdomain( true ).withwriteopts( false );
    ctxt.toselect_.allownonuserselectable_ = true;
    ctxt.fixTranslator( SEGYDirectSeisTrcTranslator::translKey() );
    outscanfld_ = new uiSeisSel( this, ctxt, scansu );
    outscanfld_->attach( alignedBelow, attgrp );
    if ( !is2d )
	outscanfld_->setInputText( objname_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, true,
					     Batch::JobSpec::SEGY );
    batchfld_->setJobName( "Read SEG-Y" );
    batchfld_->attach( alignedBelow, outimpfld_ );
}


void uiSEGYReadFinisher::cr2DCoordSrcFields( uiGroup*& attgrp, bool ismulti )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
					    tr("Coordinate source") );
    lcb->attach( alignedBelow, attgrp );
    attgrp = lcb;
    coordsfromfld_ = lcb->box();
    coordsfromfld_->setHSzPol( uiObject::WideVar );
    coordsfromfld_->selectionChanged.notify(
			mCB(this,uiSEGYReadFinisher,coordsFromChg) );
    coordsfromfld_->addItem( tr("The trace headers") );

    coordsfromfld_->addItem( ismulti
	    ? tr("'Nr X Y' files (bend-points needed)")
	    : tr("A 'Nr X Y' file (bend-points needed)") );
    if ( ismulti )
    {
	coordfileextfld_ = new uiGenInput( this, tr("File extension"),
					   StringInpSpec("crd") );
	coordfileextfld_->attach( rightOf, lcb );
    }
    else
    {
	coordfilefld_ = new uiFileInput( this, uiStrings::sFileName() );
	coordfilefld_->attach( alignedBelow, lcb );

	const Coord mincoord( SI().minCoord(true) );
	coordsfromfld_->addItem( tr("Generate straight line") );
	coordsstartfld_ = new uiGenInput( this, tr("Start coordinate"),
			DoubleInpSpec(double(mNINT64(mincoord.x))),
			DoubleInpSpec(double(mNINT64(mincoord.y))) );
	coordsstartfld_->attach( alignedBelow, lcb );
	coordsstartfld_->setElemSzPol( uiObject::Small );
	coordsstepfld_ = new uiGenInput( this, uiStrings::sStep(),
			DoubleInpSpec(mNINT32(SI().crlDistance())),
			DoubleInpSpec(0) );
	coordsstepfld_->attach( rightOf, coordsstartfld_ );
	coordsstepfld_->setElemSzPol( uiObject::Small );

	attgrp = coordfilefld_;
    }
}


void uiSEGYReadFinisher::crVSPFields()
{
    uiString inptxt( tr("Input Z (%1-%2) is") );
    const float startz = fs_.readopts_.timeshift_;
    const float endz = startz + fs_.readopts_.sampleintv_ * (fs_.pars_.ns_-1);
    inptxt.arg( startz ).arg( endz );
    const char* doms[] = { "TWT", "TVDSS", "MD", 0 };
    inpdomfld_ = new uiGenInput( this, inptxt, StringListInpSpec(doms) );
    inpdomfld_->valueChanged.notify( mCB(this,uiSEGYReadFinisher,inpDomChg) );
    isfeetfld_ = new uiCheckBox( this, tr("in Feet") );
    isfeetfld_->attach( rightOf, inpdomfld_ );
    isfeetfld_->setChecked( fs_.zinfeet_ );

    outwllfld_ = new uiWellSel( this, true, tr("Add to Well"), false );
    outwllfld_->selectionDone.notify( mCB(this,uiSEGYReadFinisher,wllSel) );
    outwllfld_->attach( alignedBelow, inpdomfld_ );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("New log name") );
    lcb->attach( alignedBelow, outwllfld_ );
    lognmfld_ = lcb->box();
    lognmfld_->setReadOnly( false );
    lognmfld_->setText( objname_ );
}


uiSEGYReadFinisher::~uiSEGYReadFinisher()
{
}


void uiSEGYReadFinisher::initWin( CallBacker* )
{
    inpDomChg( 0 );
    coordsFromChg( 0 );
    doScanChg( 0 );
}


void uiSEGYReadFinisher::wllSel( CallBacker* )
{
    if ( !lognmfld_ )
	return;

    BufferStringSet nms; Well::MGR().getLogNamesByID( outwllfld_->key(), nms );
    BufferString curlognm = lognmfld_->text();
    lognmfld_->setEmpty();
    lognmfld_->addItems( nms );
    if ( curlognm.isEmpty() )
	curlognm = "VSP";
    lognmfld_->setText( curlognm );
}


void uiSEGYReadFinisher::inpDomChg( CallBacker* )
{
    if ( !isfeetfld_ )
	return;

    isfeetfld_->display( inpdomfld_->getIntValue() != 0 );
}


void uiSEGYReadFinisher::coordsFromChg( CallBacker* )
{
    if ( !coordsfromfld_ )
	return;

    const int selidx = coordsfromfld_->currentItem();
    if ( coordfileextfld_ )
	coordfileextfld_->display( selidx == 1 );
    else
    {
	coordfilefld_->display( selidx == 1 );
	coordsstartfld_->display( selidx == 2 );
	coordsstepfld_->display( selidx == 2 );
    }
}


void uiSEGYReadFinisher::doScanChg( CallBacker* )
{
    const bool copy = docopyfld_ ? docopyfld_->getBoolValue() : true;
    if ( outimpfld_ )
	outimpfld_->display( copy );
    if ( transffld_ )
	transffld_->display( copy );
    if ( remnullfld_ )
	remnullfld_->display( !copy );
    if ( outscanfld_ )
	outscanfld_->display( !copy );
    if ( lnmfld_ )
	lnmfld_->display( !copy );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSEGYReadFinisher::doVSP()
{
    const IOObj* wllioobj = outwllfld_->ioobj();
    if ( !wllioobj )
	return false;
    const BufferString lognm( lognmfld_->text() );
    if ( lognm.isEmpty() )
	mErrRet(uiStrings::phrEnter(tr("a valid name for the new log")))

    PtrMan<SEGYSeisTrcTranslator> trl = SEGYSeisTrcTranslator::getInstance();
    trl->filePars() = fs_.pars_; trl->fileReadOpts() = fs_.readopts_;
    PtrMan<IOObj> seisioobj = fs_.spec_.getIOObj( true );
    SeisTrc trc;
    if (   !trl->initRead( seisioobj->getConn(Conn::Read), Seis::Scan )
	|| !trl->read(trc) )
	mErrRet(trl->errMsg())

    const int idom = inpdomfld_->getIntValue();
    const bool isdpth = idom > 0;
    const bool ismd = idom == 2;
    const bool inft = isdpth && isfeetfld_->isChecked();

    const MultiID& wllkey( wllioobj->key() );
    const bool wasloaded = Well::MGR().isLoaded( wllkey );
    RefMan<Well::Data> wd = Well::MGR().get( wllkey );
    if ( !wd )
	mErrRet(tr("Cannot load the selected well\n%1")
			.arg(Well::MGR().errMsg()))
    else if ( !isdpth && !wd->d2TModel() )
	mErrRet(tr("Selected well has no Depth vs Time model"))

    const Well::Track& track = wd->track();
    int wlidx = wd->logs().indexOf( lognm );
    if ( wlidx >= 0 )
	delete wd->logs().remove( wlidx );

    Well::Log* wl = new Well::Log( lognm );
    wl->pars().set( sKey::FileName(), fs_.spec_.fileName() );
    wd->logs().add( wl );

    float prevdah = mUdf(float);
    for ( int isamp=0; isamp<trc.size(); isamp++ )
    {
	float z = trc.samplePos( isamp );
	if ( !isdpth )
	    z = wd->d2TModel()->getDah( z/1000.f, track );
	else
	{
	    if ( inft )
		z *= mFromFeetFactorF;
	    if ( !ismd )
		prevdah = z = track.getDahForTVD( z, prevdah );
	}
	wl->addValue( z, trc.get(isamp,0) );
    }

    Well::Writer wwr( wllkey, *wd );
    if ( !wwr.putLog(*wl) )
	mErrRet( wwr.errMsg() )
    else if ( wasloaded )
	Well::MGR().reload( wllkey );

    return true;
}


void uiSEGYReadFinisher::updateInIOObjPars( IOObj& inioobj,
					    const IOObj& outioobj )
{
    fs_.fillPar( inioobj.pars() );
    const bool outissidom = ZDomain::isSI( outioobj.pars() );
    if ( !outissidom )
	ZDomain::Def::get(outioobj.pars()).set( inioobj.pars() );
    IOM().commitChanges( inioobj );
}


void uiSEGYReadFinisher::setCoordSystem( Coords::CoordSystem* crs )
{
    coordsys_ = crs;
}


SeisStdImporterReader* uiSEGYReadFinisher::getImpReader( const IOObj& ioobj,
			    SeisTrcWriter& wrr, Pos::GeomID geomid )
{
    const Seis::GeomType gt = wrr.geomType();
    SeisStoreAccess::Setup ssasu( ioobj, geomid, &gt );
    if ( coordsys_ )
	ssasu.coordsys( *coordsys_ );

    auto* rdr = new SeisStdImporterReader( ssasu, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->getScaler() );
    PtrMan<Seis::SelData> sd = transffld_->getSelData();
    if ( sd && !sd->isAll() )
    {
	rdr->setSelData( sd->clone() );
	wrr.setSelData( sd.release() );
    }

    return rdr;
}


bool uiSEGYReadFinisher::do3D( const IOObj& inioobj, const IOObj& outioobj,
			       bool doimp )
{
    Executor* exec;
    const Seis::GeomType gt = fs_.geomType();
    PtrMan<SeisImporter> imp;
    PtrMan<SeisTrcWriter> wrr;
    PtrMan<SEGY::FileIndexer> indexer;
    if ( doimp )
    {
	wrr = new SeisTrcWriter( outioobj, &gt );
	imp = new SeisImporter(
			getImpReader(inioobj,*wrr,Survey::default3DGeomID()),
			*wrr, gt );
	exec = imp.ptr();
    }
    else
    {
	IOPar segypars = inioobj.pars();
	if ( coordsys_ )
	    coordsys_->fillPar( segypars );

	indexer = new SEGY::FileIndexer( outioobj.key(), !Seis::isPS(gt),
			fs_.spec_, false, segypars );
	const bool discardnull =
		remnullfld_ ? remnullfld_->getBoolValue() : false;
	indexer->scanner()->fileDataSet().setDiscardNull( discardnull );
	exec = indexer.ptr();
    }

    uiTaskRunner dlg( this );
    if ( !dlg.execute(*exec) )
	return false;

    if ( imp )
    {
	mDynamicCastGet(const SeisStdImporterReader*,stdimprdr,&imp->reader())
	if ( stdimprdr )
	    SEGYSeisTrcTranslator::writeSEGYHeader( stdimprdr->reader(),
						    outioobj.fullUserExpr() );
    }

    wrr = nullptr;  // closes output
    if ( !handleWarnings(!doimp,indexer,imp) )
	{ IOM().permRemove( outioobj.key() ); return false; }

    if ( indexer )
    {
	IOPar inpiop; fs_.fillPar( inpiop );
	IOPar rep( "SEG-Y scan report" );
	indexer->scanner()->getReport( rep, &inpiop );
	uiSEGY::displayReport( parent(), rep );
    }

    uiSeisIOObjInfo oinf( outioobj, true );
    return oinf.provideUserInfo();
}


bool uiSEGYReadFinisher::getGeomID( const char* lnm, bool isnew,
				    Pos::GeomID& geomid ) const
{
    uiString errmsg =
	    tr("Internal: Cannot create line geometry in database");
    geomid = Survey::GM().getGeomID( lnm );
    if ( isnew )
	geomid = Geom2DImpHandler::getGeomID( lnm );

    if ( geomid == mUdfGeomID )
	mErrRet( errmsg )

    return true;
}


bool uiSEGYReadFinisher::do2D( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp, const char* inplnm )
{
    const int nrlines = fs_.spec_.nrFiles();
    bool overwr_warn = true, overwr = false, needupdate = false;
    TypeSet<Pos::GeomID> geomids;
    for ( int iln=0; iln<nrlines; iln++ )
    {
	BufferString lnm( inplnm );
	if ( nrlines > 1 )
	{
	    if ( iln >= fs_.linenames_.size() )
		break;

	    lnm = fs_.linenames_.get( iln );
	}

	const bool morelines = iln < nrlines-1;
	bool isnew = true;
	if ( !handleExistingGeometry(lnm,morelines,overwr_warn,overwr,isnew) )
	    return false;

	Pos::GeomID geomid;
	if ( !getGeomID(lnm,isnew,geomid) )
	    return false;

	const BufferString fnm( fs_.spec_.fileName(iln) );
	if ( !exec2Dimp(inioobj,outioobj,doimp,fnm,lnm,geomid) )
	    return false;

	geomids += geomid;
	if ( isnew || overwr )
	    needupdate = true;
    }

    if ( needupdate )
	Survey::GMAdmin().updateGeometries( nullptr );

    uiSeisIOObjInfo oinf( outioobj, true );
    return oinf.provideUserInfo2D( &geomids );
}


bool uiSEGYReadFinisher::doBatch( bool doimp )
{
    const Seis::GeomType gt = fs_.geomType();
    const BufferString jobname( doimp ? "Import_SEG-Y_" : "Scan_SEG-Y_",
				outFld(doimp)->getInput() );
    batchfld_->setJobName( jobname );
    IOPar& jobpars = batchfld_->jobSpec().pars_;
    jobpars.setEmpty();
    Seis::putInPar( gt, jobpars );
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
			   : (Seis::isPS(gt) ? SEGY::IO::sKeyIndexPS()
					     : SEGY::IO::sKeyIndex3DVol()) );
    IOPar inpars;
    fs_.fillPar( inpars );
    if ( coordsys_ )
	coordsys_->fillPar( inpars );
    jobpars.mergeComp( inpars, sKey::Input() );

    IOPar outpars;
    if ( transffld_ )
	transffld_->fillPar( outpars );

    uiSeisSel* seisselfld = outFld(doimp);
    if ( seisselfld )
	seisselfld->fillPar( outpars );

    jobpars.mergeComp( outpars, sKey::Output() );

    return batchfld_->start();
}


bool uiSEGYReadFinisher::doBatch2D( bool doimp, const char* inplnm )
{
    const Seis::GeomType gt = fs_.geomType();
    uiSeisSel* seisselfld = outFld(doimp);
    const int nrlines = fs_.spec_.nrFiles();
    const BufferString infostr( nrlines > 1 ? "multi" : inplnm, "_",
				seisselfld->getInput() );
    const BufferString jobname( doimp ? "Import_SEG-Y_" : "Scan_SEG-Y_",
				infostr );
    batchfld_->setJobName( jobname );
    IOPar& jobpars = batchfld_->jobSpec().pars_;
    jobpars.setEmpty();
    Seis::putInPar( gt, jobpars );
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
			   : (Seis::isPS(gt) ? SEGY::IO::sKeyIndexPS()
					     : SEGY::IO::sKeyIndex3DVol()) );

    IOPar inpars;
    fs_.fillPar( inpars );
    if ( coordsys_ )
	coordsys_->fillPar( inpars );

    bool overwr_warn = true; bool overwr = false;
    for ( int iln=0; iln<nrlines; iln++ )
    {
	BufferString lnm( inplnm );
	if ( nrlines > 1 )
	    lnm = fs_.linenames_.get( iln );

	const bool morelines = iln < nrlines-1;
	bool isnew = true;
	if ( !handleExistingGeometry(lnm,morelines,overwr_warn,overwr,isnew) )
	    return false;

	Pos::GeomID geomid;
	if ( !getGeomID(lnm,isnew,geomid) )
	    return false;

	if ( iln==0 )
	    inpars.set( sKey::GeomID(), geomid );
	else
	    inpars.set( IOPar::compKey(sKey::GeomID(),iln), geomid );
    }

    jobpars.mergeComp( inpars, sKey::Input() );

    IOPar outpars;
    if ( transffld_ )
	transffld_->fillPar( outpars );

    seisselfld->fillPar( outpars );
    jobpars.mergeComp( outpars, sKey::Output() );

    return batchfld_->start();
}


bool uiSEGYReadFinisher::exec2Dimp( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp, const char* fnm, const char* lnm,
				Pos::GeomID geomid )
{
    Executor* exec;
    const Seis::GeomType gt = fs_.geomType();
    PtrMan<SeisTrcWriter> wrr; PtrMan<SeisImporter> imp;
    PtrMan<SEGY::FileIndexer> indexer; PtrMan<IOStream> iniostrm;
    SEGY::FileSpec fspec( fs_.spec_ );
    fspec.setFileName( fnm );
    if ( doimp )
    {
	iniostrm = static_cast<IOStream*>( fspec.getIOObj( true ) );
	updateInIOObjPars( *iniostrm, outioobj );
	wrr = new SeisTrcWriter( outioobj, geomid, &gt );
	imp = new SeisImporter( getImpReader(*iniostrm,*wrr,geomid), *wrr, gt );
	BufferString nm( imp->name() ); nm.add( " (" ).add( lnm ).add( ")" );
	imp->setName( nm );
	exec = imp.ptr();
    }
    else
    {
	IOPar segypars = inioobj.pars();
	segypars.set( sKey::GeomID(), geomid );
	if ( coordsys_ )
	    coordsys_->fillPar( segypars );

	indexer = new SEGY::FileIndexer( outioobj.key(), !Seis::isPS(gt),
					 fspec, true, segypars );
	exec = indexer.ptr();
    }

    uiTaskRunner dlg( this );
    if ( !dlg.execute(*exec) )
	return false;

    wrr.erase(); // closes output
    handleWarnings( false, indexer, imp );

    return true;
}


bool uiSEGYReadFinisher::handleExistingGeometry( const char* lnm, bool morelns,
					     bool& overwr_warn, bool& overwr,
					     bool& isnewline )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
    if ( geomid == mUdfGeomID )
	return true;

    isnewline = false;
    int choice = overwr ? 1 : 2;
    if ( overwr_warn )
    {
	uiDialog::Setup dsu( tr("Overwrite geometry?"),
		tr("Geometry of Line '%1' is already present."
		"\n\nDo you want to overwrite it?").arg(lnm),
		mODHelpKey(mhandleExistingGeometryHelpID));
	uiDialog dlg( this, dsu );
	uiCheckList* optfld = new uiCheckList( &dlg, uiCheckList::OneOnly );
	optfld->addItem( tr("Cancel: stop the import"), "cancel" );
	optfld->addItem( tr("Yes: set the geometry from the SEG-Y file"),
							"checkgreen" );
	optfld->addItem( tr("No: keep the existing geometry"), "handstop" );
	if ( morelns )
	{
	    optfld->addItem( tr("All: overwrite all existing geometries"),
				"doall" );
	    optfld->addItem( tr("None: keep all existing geometries"),
				"donone" );
	    choice = overwr ? 3 : 4;
	}
	optfld->setChecked( choice, true );
	if ( !dlg.go() || optfld->firstChecked() == 0 )
	    return false;

	choice = optfld->firstChecked();
	if ( choice > 2 )
	    overwr_warn = false;
    }

    overwr = choice == 1 || choice == 3;
    if ( overwr )
    {
	Survey::Geometry* geom = Survey::GMAdmin().getGeometry(geomid );
	mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);
	if ( geom2d )
	    geom2d->dataAdmin().setEmpty();
    }

    return true;
}


bool uiSEGYReadFinisher::putCoordChoiceInSpec()
{
    const int selidx = coordsfromfld_->currentItem();
    SEGY::FileReadOpts& opts = fs_.readopts_;
    opts.coorddef_ = (SEGY::FileReadOpts::CoordDefType)selidx;
    if ( selidx < 1 )
	return true;

    if ( selidx == 1 )
    {
	if ( coordfileextfld_ )
	    opts.coordfnm_.set( "ext=" ).add( coordfileextfld_->text() );
	else
	{
	    const BufferString fnm( coordfilefld_->fileName() );
	    if ( fnm.isEmpty() || !File::exists(fnm) )
	    {
		mErrRet(
		    tr("Please choose an existing file for the coordinates") )
		return false;
	    }
	    opts.coordfnm_.set( fnm );
	}
    }
    else
    {
	const Coord startcrd( coordsstartfld_->getCoord() );
	if ( mIsUdf(startcrd.x) || mIsUdf(startcrd.y) )
	    mErrRet(uiStrings::phrEnter(tr("the start coordinate")))
	else if ( !SI().isReasonable(startcrd) )
	    mErrRet(tr("The start coordinate is too far from the survey"))

	Coord stepcrd( coordsstepfld_->getCoord() );
	if ( mIsUdf(stepcrd.x) ) stepcrd.x = 0;
	if ( mIsUdf(stepcrd.y) ) stepcrd.y = 0;
	if ( mIsZero(stepcrd.x,0.001) && mIsZero(stepcrd.y,0.001) )
	    mErrRet(tr("The steps cannot both be zero"))

	opts.startcoord_ = startcrd;
	opts.stepcoord_ = stepcrd;
    }

    return true;
}


bool uiSEGYReadFinisher::handleWarnings( bool withstop,
				SEGY::FileIndexer* indexer, SeisImporter* imp )
{
    BufferStringSet warns;
    if ( indexer )
	warns.add( indexer->scanner()->warnings(), false );
    else
    {
	if ( imp->nrSkipped() > 0 )
	    warns += new BufferString("[9] During import, ", imp->nrSkipped(),
				      " traces were rejected" );
	SeisStdImporterReader& stdrdr
		= static_cast<SeisStdImporterReader&>( imp->reader() );
	SeisTrcTranslator* transl = stdrdr.reader().seisTranslator();
	if ( transl && transl->haveWarnings() )
	    warns.add( transl->warnings(), false );
    }

    return uiSEGY::displayWarnings( warns, withstop );
}


void uiSEGYReadFinisher::setAsDefaultObj()
{
    if ( fs_.isVSP() || outputid_.isUdf() )
	return;

    PtrMan<IOObj> ioobj = IOM().get( outputid_ );
    if ( ioobj )
	ioobj->setSurveyDefault();
}


bool uiSEGYReadFinisher::acceptOK( CallBacker* )
{
    outputid_.setUdf();
    if ( fs_.isVSP() )
	return doVSP();

    const bool doimp = docopyfld_ ? docopyfld_->getBoolValue() : true;
    const IOObj* ioobj = outFld(doimp)->ioobj();
    if ( !ioobj )
	return false;

    PtrMan<IOObj> outioobj = ioobj->clone();
    outputid_ = outioobj->key();

    const bool is2d = Seis::is2D( fs_.geomType() );
    BufferString lnm;
    if ( is2d )
    {
	lnm = doimp ? transffld_->selFld2D()->selectedLine()
		    : lnmfld_->getInput();
	if ( lnm.isEmpty() )
	    mErrRet( uiStrings::phrEnter(tr("a line name")) )
	if ( !putCoordChoiceInSpec() )
	    return false;
    }

    if ( doimp && !Seis::isPS(fs_.geomType()) )
    {
	uiSeisIOObjInfo oinf( *outioobj, true );
	if ( !oinf.checkSpaceLeft(transffld_->spaceInfo()) )
	    return false;
    }

    if ( !doimp )
    {
	const bool ismulti = fs_.spec_.nrFiles() > 1;
	uiString part = ismulti ? tr("files remain") : tr("file remains");
	uiString msg = tr("You have selected the SEGYDirect (scan&link) "
		"import option.\n"
		"Please make sure that the SEG-Y %1\n"
		"accessible while using OpendTect.\n\n"
		"Do you want to continue the import as SEGYDirect?").arg(part);
	const bool res = uiMSG().askContinue( msg );
	if ( !res )
	    return false;
    }

    if ( batchfld_ && batchfld_->wantBatch() )
	return is2d ? doBatch2D( doimp, lnm ) : doBatch( doimp );

    PtrMan<IOObj> inioobj = fs_.spec_.getIOObj( true );
    updateInIOObjPars( *inioobj, *outioobj );

    const bool res = is2d ? do2D( *inioobj, *outioobj, doimp, lnm )
			  : do3D( *inioobj, *outioobj, doimp );
    IOM().permRemove( inioobj->key() );
    return res;
}
