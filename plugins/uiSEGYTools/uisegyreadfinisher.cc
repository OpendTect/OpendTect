/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/

#include "uisegyreadfinisher.h"

#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uicombobox.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uiwellsel.h"

#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "keystrs.h"
#include "posinfo2dsurv.h"
#include "segybatchio.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segydirect2d.h"
#include "segyscanner.h"
#include "segytr.h"
#include "segyvintageimporter.h"
#include "seisimporter.h"
#include "seisioobjinfo.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "survgeommgr.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmanager.h"
#include "welltrack.h"


uiString uiSEGYReadFinisher::getWinTile( const FullSpec& fs, bool issingle )
{
    uiString ret;
    if ( !issingle )
    {
	ret = tr("Select options for importing bulk segy data");
	return ret;
    }
    const Seis::GeomType gt = fs.geomType();
    const bool isvsp = fs.isVSP();

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
				const char* usrspec, bool istime,
				bool singlevintage,
				const ObjectSet<SEGY::Vintage::Info>* vntinfo)
    : uiDialog(p,uiDialog::Setup(getWinTile(fs,singlevintage),
	       singlevintage ? getDlgTitle(usrspec) : mNoDlgTitle,
	       mODHelpKey(mSEGYReadFinisherHelpID)))
    , fs_(fs)
    , outwllfld_(0)
    , lognmfld_(0)
    , inpdomfld_(0)
    , isfeetfld_(0)
    , outimpfld_(0)
    , outscanfld_(0)
    , transffld_(0)
    , lnmfld_(0)
    , batchfld_(0)
    , docopyfld_(0)
    , coordsfromfld_(0)
    , coordfilefld_(0)
    , coordfileextfld_(0)
    , singlevintage_(singlevintage)
    , vntinfos_(vntinfo)
    , updateStatus(this)
{
    setOkText( uiStrings::sImport() );
    if ( !singlevintage )
	setCancelText( uiStrings::sWizBack() );

    objname_ = File::Path( usrspec ).baseName();
    const bool is2d = Seis::is2D( fs_.geomType() );
    if ( !is2d )
	objname_.replace( '*', 'x' );
    if ( !singlevintage  && !is2d )
	objname_ = "From SEGY file names";

    if ( fs_.isVSP() )
	crVSPFields( istime );
    else
	crSeisFields( istime );

    postFinalise().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


void uiSEGYReadFinisher::crSeisFields( bool istime )
{
    const Seis::GeomType gt = fs_.geomType();
    const bool is2d = Seis::is2D( gt );
    const bool ismulti = fs_.spec_.nrFiles() > 1;

    docopyfld_ = new uiGenInput( this, uiStrings::phrCopy(
				 uiStrings::sData().toLower()),
	    BoolInpSpec(true,tr("Yes (import)"),tr("No (scan&&link)")) );
    docopyfld_->valuechanged.notify(mCB(this,uiSEGYReadFinisher,doScanChg));

    uiSeisTransfer::Setup trsu( gt );
    trsu.withnullfill( false ).fornewentry( true );
    trsu.multiline( is2d ? ismulti : false );
    transffld_ = new uiSeisTransfer( this, trsu );
    transffld_->attach( alignedBelow, docopyfld_ );
    if ( is2d )
    {
	lnmfld_ = new uiSeis2DLineNameSel( this, false );
	lnmfld_->attach( alignedBelow, docopyfld_ );

	uiSeis2DSubSel& selfld = *transffld_->selFld2D();
	if ( ismulti || !singlevintage_ )
	{
	    selfld.setSelectedLine( "*" );
	    selfld.setSensitive( false );
	    lnmfld_->setInput( "*" );
	    lnmfld_->setSensitive( false );
	}
	else
	{
	    selfld.setSelectedLine( objname_ );
	    lnmfld_->setInput( objname_ );
	}
    }

    uiGroup* attgrp = transffld_;
    if ( is2d )
	cr2DCoordSrcFields( attgrp, ismulti );

    uiString lbl = uiStrings::phrOutput(
		uiStrings::sSeisObjName( is2d, !is2d, false, false, false ));
    uiSeisSel::Setup copysu( gt );
    copysu.seltxt( lbl );
    copysu.optionsselectable( singlevintage_ || is2d );
    copysu.enabotherdomain( singlevintage_ )
	  .isotherdomain( istime != SI().zIsTime() );
    IOObjContext ctxt( uiSeisSel::ioContext( gt, false ) );
    outimpfld_ = new uiSeisSel( this, ctxt, copysu );
    outimpfld_->attach( alignedBelow, attgrp );
    if ( !is2d )
	outimpfld_->setInputText( objname_ );

    uiSeisSel::Setup scansu( copysu );
    scansu.withwriteopts( false );
    ctxt.toselect_.allownonuserselectable_ = true;
    ctxt.fixTranslator( SEGYDirectSeisTrcTranslator::translKey() );
    outscanfld_ = new uiSeisSel( this, ctxt, scansu );
    outscanfld_->attach( alignedBelow, attgrp );
    if ( !is2d )
	outscanfld_->setInputText( objname_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, true,
					     Batch::JobSpec::SEGY );
    batchfld_->setJobName( "Read SEG-Y" );
    batchfld_->jobSpec().pars_.setYN( SEGY::IO::sKeyIs2D(), is2d );
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
	coordfilefld_ = new uiFileSel( this, uiStrings::sFileName() );
	coordfilefld_->attach( alignedBelow, lcb );

	const Coord mincoord( SI().minCoord(OD::UsrWork) );
	coordsfromfld_->addItem( tr("Generate straight line") );
	coordsstartfld_ = new uiGenInput( this, tr("Start coordinate"),
			DoubleInpSpec((double)mNINT64(mincoord.x_)),
			DoubleInpSpec((double)mNINT64(mincoord.y_)) );
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


void uiSEGYReadFinisher::crVSPFields( bool istime )
{
    uiString inptxt( tr("Input Z (%1-%2) is") );
    const float startz = fs_.readopts_.timeshift_;
    const float endz = startz + fs_.readopts_.sampleintv_ * (fs_.pars_.ns_-1);
    inptxt.arg( startz ).arg( endz );
    uiStringSet domstrs;
    domstrs.add( uiStrings::sTWT() )
	   .add( uiStrings::sTVDSS() )
	   .add( uiStrings::sMD() );
    inpdomfld_ = new uiGenInput( this, inptxt, StringListInpSpec(domstrs) );
    if ( !istime )
	inpdomfld_->setValue( 1 );
    inpdomfld_->valuechanged.notify( mCB(this,uiSEGYReadFinisher,inpDomChg) );
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
    deepErase( reports_ );
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

    BufferStringSet nms; Well::MGR().getLogNames( outwllfld_->key(), nms );
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

    const DBKey wllkey( wllioobj->key() );
    uiRetVal uirv;
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( wllkey, Well::LoadReqs(),
						      uirv );
    if ( !wd )
	mErrRet( uirv )

    const Well::Track& track = wd->track();
    Well::Log* wl = new Well::Log( lognm );
    wl->pars().set( sKey::FileName(), fs_.spec_.fileName() );
    wd->logs().add( wl );

    float prevdah = mUdf(float);
    for ( int isamp=0; isamp<trc.size(); isamp++ )
    {
	float z = trc.samplePos( isamp );
	if ( !isdpth )
	    z = wd->d2TModel().getDah( z, track );
	else
	{
	    if ( inft )
		z *= mFromFeetFactorF;
	    if ( !ismd )
		prevdah = z = track.getDahForTVD( z, prevdah );
	}
	wl->addValue( z, trc.get(isamp,0) );
    }

    SilentTaskRunnerProvider trprov;
    uirv = Well::MGR().save( wllkey, trprov );
    if ( uirv.isError() )
	mErrRet( uirv )

    return true;
}


void uiSEGYReadFinisher::updateInIOObjPars( IOObj& inioobj,
					    const IOObj& outioobj,
					    const char* lnm )
{
    fs_.fillPar( inioobj.pars() );
    const bool outissidom = ZDomain::isSI( outioobj.pars() );
    if ( !outissidom )
	ZDomain::Def::get(outioobj.pars()).set( inioobj.pars() );
    if ( lnm )
	inioobj.pars().set( sKey::LineName(), lnm );
    inioobj.commitChanges();
}


SeisStdImporterReader* uiSEGYReadFinisher::getImpReader( const IOObj& ioobj,
			    Seis::Storer& storer, Pos::GeomID geomid )
{
    SeisStdImporterReader* rdr = new SeisStdImporterReader( ioobj, "SEG-Y",
							    true );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->getScaler() );
    Seis::SelData* sd = transffld_->getSelData();
    if ( sd )
    {
	if ( sd->isRange() )
	    sd->asRange()->setGeomID( geomid );
	rdr->setSelData( sd );
    }
    return rdr;
}


bool uiSEGYReadFinisher::doMultiVintage( const char* attr2dnm )
{
    if ( singlevintage_ )
	return false;

    if ( !vntinfos_ && !vntinfos_->size() )
	return false;

    for ( int vidx=0; vidx<vntinfos_->size(); vidx++ )
    {
	processingvntnm_.setEmpty();
	uiSEGYImportResult* reportdlg = new uiSEGYImportResult( this );
	reportdlg->vintagenm_ = vntinfos_->get(vidx)->vintagenm_;
	reports_.add( reportdlg );
	reportdlg->status_ = uiStrings::sImporting();
	processingvntnm_ = vntinfos_->get(vidx)->vintagenm_;
	updateStatus.trigger();
	if ( !outimpfld_->getTranslator() )
	    return false;

	IOObjContext* ctxt = Seis::getIOObjContext( fs_.geomType(), false );
	const Translator* transl = outimpfld_->getTranslator();
	ctxt->fixTranslator( transl->userName().buf() );

	const SEGY::Vintage::Info* vntinfo = vntinfos_->get(vidx);
	Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
	if ( parset.isEmpty() )
	    continue;

	SEGY::Vintage::Importer vntimporter( *vntinfo, transl->userName(),
					     fs_.geomType(),
					     transffld_->getSelData(),
					     attr2dnm );
	vntimporter.setContinueOnError( true );
	uiTaskRunner dlg( this, singlevintage_ );
	dlg.execute( vntimporter );
	updateResultDlg( vntimporter, reportdlg );
	reportdlg->status_ = uiStrings::sFinished();
	updateStatus.trigger();
    }

    processingvntnm_.setEmpty();
    return true;
}


const SEGY::Vintage::Info* uiSEGYReadFinisher::getVintageInfo(
						     const BufferString& vntnm )
{
    if ( !vntinfos_ || vntinfos_->isEmpty() )
	return 0;

    for ( int vint=0; vint<vntinfos_->size(); vint++ )
    {
	const SEGY::Vintage::Info* vinfo = vntinfos_->get( vint );
	if ( !vinfo )
	    continue;

	if ( vinfo->vintagenm_.isEqual(vntnm) )
	    return vinfo;
    }

    return 0;
}


void uiSEGYReadFinisher::updateResultDlg(
				      const SEGY::Vintage::Importer& importer,
				      uiSEGYImportResult* reportdlg )
{
    const SEGY::Vintage::Info* vntinfo = getVintageInfo( importer.getName() );
    if ( !vntinfo )
	return;

    reportdlg->vintagenm_ = importer.getName();
    int nrexes = importer.nrExecutors();
    if ( nrexes != mNINT64(vntinfo->filenms_.size()) )
	return;

    for ( int idx=0; idx<importer.nrExecutors(); idx++ )
    {
	const int nrrows = reportdlg->table_->nrRows();
	reportdlg->table_->setNrRows( nrrows + 1 );
	const int currowid = reportdlg->table_->nrRows() - 1;
	const BufferString fnm( vntinfo->filenms_.get(idx) );
	reportdlg->table_->setText( RowCol(currowid,0), fnm );
	uiString tooltip;
	File::Path fp( vntinfo->fp_.pathOnly(), fnm );
	tooltip = toUiString( fp.fullPath() );
	reportdlg->table_->setCellToolTip( RowCol(currowid,0), tooltip );
	uiTextEdit* txt = new uiTextEdit( this, "", true );
	txt->setDefaultHeight( 25 );
	mDynamicCastGet(const SeisImporter*,seisimp,importer.getExecutor(idx));
	if ( !seisimp )
	    continue;

	bool impsuccess = seisimp->errorMsg().isEmpty();
	txt->setText( impsuccess ? tr("Imported successfully")
				 : seisimp->errorMsg() );
	reportdlg->table_->setRowStretchable( currowid, true );
	reportdlg->table_->setCellObject( RowCol(currowid,2), txt );
	reportdlg->table_->setText( RowCol(currowid,1),
				    impsuccess ? uiStrings::sSuccess()
					       : uiStrings::sFailed() );
	reportdlg->table_->setCellColor( RowCol(currowid,1),
					 impsuccess ? Color::Green()
						    : Color::Red() );

    }
}


bool uiSEGYReadFinisher::do3D( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp )
{
    Executor* exec;
    const Seis::GeomType gt = fs_.geomType();
    PtrMan<Seis::Storer> storer; PtrMan<SeisImporter> imp;
    PtrMan<SEGY::FileIndexer> indexer;
    if ( doimp )
    {
	storer = new Seis::Storer( outioobj );
	imp = new SeisImporter( getImpReader(inioobj,*storer,mUdfGeomID),
				*storer, gt );
	exec = imp.ptr();
    }
    else
    {
	indexer = new SEGY::FileIndexer( outioobj.key(), !Seis::isPS(gt),
			fs_.spec_, false, inioobj.pars() );
	exec = indexer.ptr();
    }

    uiTaskRunner dlg( this, singlevintage_ );
    if ( !dlg.execute( *exec ) )
    {
	if ( !singlevintage_ && doimp )
	    errmsg_.appendPhrase( imp.ptr()->message(), uiString::NoSep );

	return false;
    }
    else
    {
	if ( !singlevintage_ && doimp )
	{
	    trcsskipped_ = imp.ptr()->nrSkipped() > 0;
	    if ( imp.ptr()->nrSkipped() > 0 )
		errmsg_.appendPhrase( tr("Warning: %1")
						.arg(imp.ptr()->message()) );
	    else
		errmsg_.appendPhrase( uiStrings::sImpSuccess(),
							    uiString::NoSep );
	}
    }

    if ( storer )
	storer->close();
    if ( singlevintage_ && !handleWarnings(!doimp,indexer,imp) )
	{ outioobj.removeFromDB(); return false; }

    if ( indexer )
    {
	IOPar inpiop; fs_.fillPar( inpiop );
	IOPar rep( "SEG-Y scan report" );
	indexer->scanner()->getReport( rep, &inpiop );
	uiSEGY::displayReport( parent(), rep );
    }

    uiSeisIOObjInfo oinf( this, outioobj );
    return oinf.provideUserInfo();
}


bool uiSEGYReadFinisher::getGeomID( const char* lnm, bool isnew,
				    Pos::GeomID& geomid ) const
{
    const bool doimp = docopyfld_ ? docopyfld_->getBoolValue() : true;
    uiString errmsg =
	    tr("Internal: Cannot create line geometry in database");
    geomid = SurvGeom::getGeomID( lnm );
    if ( isnew && !doimp )
    {
	PtrMan<IOObj> geomobj = SurvGeom2DTranslator::getEntry( lnm,
			    SEGYDirectSurvGeom2DTranslator::translKey() );
	if ( !geomobj )
	    mErrRet( errmsg );

	geomobj->pars().set(
		    SEGYDirectSurvGeom2DTranslator::sKeySEGYDirectID(),
		    outscanfld_->key(true) );
	geomobj->commitChanges();
	geomid = SurvGeom2DTranslator::getGeomID( *geomobj );
    }
    else if ( isnew )
	geomid = Geom2DImpHandler::getGeomID( lnm );

    if ( geomid == mUdfGeomID )
	mErrRet( errmsg )

    return true;
}


bool uiSEGYReadFinisher::do2D( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp, const char* inplnm )
{
    const int nrlines = fs_.spec_.nrFiles();
    bool overwr_warn = true; bool overwr = !Seis::isPS( fs_.geomType() );
    GeomIDSet geomids;
    for ( int iln=0; iln<nrlines; iln++ )
    {
	BufferString lnm( inplnm );
	if ( nrlines > 1 )
	    lnm = getWildcardSubstLineName( iln );

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
    }

    Survey::GMAdmin().updateGeometries( SilentTaskRunnerProvider() );
    uiSeisIOObjInfo oinf( this, outioobj );
    return oinf.provideLineInfo( &geomids );
}


bool uiSEGYReadFinisher::doBatch( bool doimp )
{
    const BufferString jobname( doimp ? "Import_SEG-Y_" : "Scan_SEG-Y_",
				outFld(doimp)->getInput() );
    batchfld_->setJobName( jobname );
    IOPar& jobpars = batchfld_->jobSpec().pars_;
    const bool isps = Seis::isPS( fs_.geomType() );
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
	    : (isps ? SEGY::IO::sKeyIndexPS() : SEGY::IO::sKeyIndex3DVol()) );
    fs_.fillPar( jobpars );

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
    const int nrlines = fs_.spec_.nrFiles();
    const BufferString infostr( nrlines ? "multi" : inplnm, "_",
				outFld(doimp)->getInput() );
    const BufferString jobname( doimp ? "Import_SEG-Y_" : "Scan_SEG-Y_",
				infostr );
    batchfld_->setJobName( jobname );
    IOPar& jobpars = batchfld_->jobSpec().pars_;
    const bool isps = Seis::isPS( fs_.geomType() );
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
	    : (isps ? SEGY::IO::sKeyIndexPS() : SEGY::IO::sKeyIndex3DVol()) );
    fs_.fillPar( jobpars );

    bool overwr_warn = true; bool overwr = !Seis::isPS( fs_.geomType() );
    for ( int iln=0; iln<nrlines; iln++ )
    {
	BufferString lnm( inplnm );
	if ( nrlines > 1 )
	    lnm = getWildcardSubstLineName( iln );

	const bool morelines = iln < nrlines-1;
	bool isnew = true;
	if ( !handleExistingGeometry(lnm,morelines,overwr_warn,overwr,isnew) )
	    return false;

	Pos::GeomID geomid;
	if ( !getGeomID(lnm,isnew,geomid) )
	    return false;

	if ( iln==0 )
	    jobpars.set( sKey::GeomID(), geomid );
	else
	    jobpars.set( IOPar::compKey(sKey::GeomID(),iln), geomid );
    }

    IOPar outpars;
    outFld(doimp)->fillPar( outpars );
    jobpars.mergeComp( outpars, sKey::Output() );

    return batchfld_->start();
}


bool uiSEGYReadFinisher::exec2Dimp( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp, const char* fnm, const char* lnm,
				Pos::GeomID geomid )
{
    Executor* exec;
    const Seis::GeomType gt = fs_.geomType();
    PtrMan<Seis::Storer> storer; PtrMan<SeisImporter> imp;
    PtrMan<SEGY::FileIndexer> indexer; PtrMan<IOStream> iniostrm;
    SEGY::FileSpec fspec( fs_.spec_ );
    fspec.setFileName( fnm );
    if ( doimp )
    {
	iniostrm = static_cast<IOStream*>( fspec.getIOObj( true ) );
	updateInIOObjPars( *iniostrm, outioobj, lnm );
	storer = new Seis::Storer( outioobj );
	imp = new SeisImporter( getImpReader(*iniostrm,*storer,geomid),
				*storer, gt );
	BufferString nm( imp->name() ); nm.add( " (" ).add( lnm ).add( ")" );
	imp->setName( nm );
	exec = imp.ptr();
    }
    else
    {
	IOPar pars = inioobj.pars();
	pars.set( sKey::GeomID(), geomid );
	indexer = new SEGY::FileIndexer( outioobj.key(), !Seis::isPS(gt),
					 fspec, true, pars );
	exec = indexer.ptr();
    }


    uiTaskRunner dlg( this );
    if ( !dlg.execute( *exec ) )
    {
	if ( iniostrm )
	    iniostrm->removeFromDB();
	return false;
    }
    if ( storer )
    {
	auto uirv = storer->close();
	if ( !uirv.isOK() )
	    uiMSG().warning( uirv );
    }

    if ( singlevintage_ )
	handleWarnings( false, indexer, imp );

    if ( iniostrm )
	iniostrm->removeFromDB();

    return true;
}


bool uiSEGYReadFinisher::handleExistingGeometry( const char* lnm, bool morelns,
					     bool& overwr_warn, bool& overwr,
					     bool& isnewline )
{
    Pos::GeomID geomid = SurvGeom::getGeomID( lnm );
    if ( mIsUdfGeomID(geomid) )
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
	const auto& geom2d = SurvGeom::get2D( geomid );
	geom2d.setEmpty();
	uiString errmsg;
	Survey::GMAdmin().save(geom2d,errmsg);
    }

    return true;
}


BufferString uiSEGYReadFinisher::getWildcardSubstLineName( int iln ) const
{
    BufferString fnm( fs_.spec_.fileName( iln ) );
    BufferString wildcardexpr = fs_.spec_.usrStr();

    char* pwc = wildcardexpr.getCStr();
    char* pfnm = fnm.getCStr();
    while ( *pwc && *pfnm && *pwc != '*' )
	{ pwc++; pfnm++; }
    if ( !*pwc || !*pfnm )
	return BufferString( "_" );

    BufferString ret;
    while ( pwc )
    {
	char* nonwcstart = pwc + 1;
	if ( !*nonwcstart )
	    break;

	pwc = firstOcc( nonwcstart, '*' ); // move to next wildcard, if there
	if ( pwc )
	    *pwc = '\0';

	char* subststart = pfnm;
	pfnm = firstOcc( pfnm, nonwcstart );
	if ( !pfnm )
	    { pErrMsg("Huh"); if ( ret.isEmpty() ) ret.set( "_" ); return ret; }
	*pfnm = '\0';
	pfnm += FixedString(nonwcstart).size(); // to next wildcard section

	if ( !ret.isEmpty() )
	    ret.add( '_' );
	ret.add( subststart );
    }
    return ret;
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
	if ( mIsUdf(startcrd.x_) || mIsUdf(startcrd.y_) )
	    mErrRet(uiStrings::phrEnter(tr("the start coordinate")))
	else if ( !SI().isReasonable(startcrd) )
	    mErrRet(tr("The start coordinate is too far from the survey"))

	Coord stepcrd( coordsstepfld_->getCoord() );
	if ( mIsUdf(stepcrd.x_) ) stepcrd.x_ = 0;
	if ( mIsUdf(stepcrd.y_) ) stepcrd.y_ = 0;
	if ( mIsZero(stepcrd.x_,0.001) && mIsZero(stepcrd.y_,0.001) )
	    mErrRet(tr("The steps cannot both be zero"))

	opts.startcoord_ = startcrd;
	opts.stepcoord_ = stepcrd;
    }

    return true;
}


bool uiSEGYReadFinisher::handleWarnings( bool withstop,
				SEGY::FileIndexer* indexer, SeisImporter* imp )
{
    uiStringSet warns; int nrskip = 0;
    if ( indexer )
	warns.add( indexer->scanner()->warnings() );
    else
	nrskip = imp->nrSkipped();

    return uiSEGY::displayWarnings( this, warns, withstop, nrskip );
}


void uiSEGYReadFinisher::setAsDefaultObj()
{
    if ( fs_.isVSP() || outputid_.isInvalid() )
	return;

    PtrMan<IOObj> ioobj = outputid_.getIOObj();
    if ( ioobj )
	ioobj->setSurveyDefault();
}


bool uiSEGYReadFinisher::acceptOK()
{
    const bool is2d = Seis::is2D( fs_.geomType() );
    if ( !singlevintage_ )
    {
	uiButton* okbut = button( uiDialog::OK );
	const char* attr2dnm = 0;
	if ( is2d )
	{
	    const IOObj* outioobj = outFld(true)->ioobj();
	    if ( !outioobj )
		return false;

	    attr2dnm = outioobj->name().buf();
	}

	const bool res = doMultiVintage( attr2dnm );
	okbut->setSensitive( res );
	return res;
    }

    outputid_.setInvalid();
    if ( fs_.isVSP() )
	return doVSP();

    const bool doimp = docopyfld_ ? docopyfld_->getBoolValue() : true;
    ConstPtrMan<IOObj> outioobj = outFld(doimp)->ioobj()->clone();
    if ( !outioobj )
	return false;

    outputid_ = outioobj->key();

    BufferString lnm;
    if ( is2d )
    {
	lnm = doimp ? transffld_->selFld2D()->selectedLine()
		    : lnmfld_->getInput();
	if ( lnm.isEmpty() && fs_.spec_.nrFiles()==1 )
	{
	    mErrRet( uiStrings::phrEnter(tr("a line name")) )
	    return false;
	}
	if ( !putCoordChoiceInSpec() )
	    return false;
    }

    if ( doimp && !Seis::isPS(fs_.geomType()) )
    {
	uiSeisIOObjInfo oinf( this, *outioobj );
	if ( !oinf.checkSpaceLeft(transffld_->spaceInfo(),true) )
	    return false;
    }

    if ( batchfld_ && batchfld_->wantBatch() )
	return is2d ? doBatch2D( doimp, lnm ) : doBatch( doimp );

    PtrMan<IOObj> inioobj = fs_.spec_.getIOObj( true );
    updateInIOObjPars( *inioobj, *outioobj, lnm );

    const bool res = is2d ? do2D( *inioobj, *outioobj, doimp, lnm )
			  : do3D( *inioobj, *outioobj, doimp );
    inioobj->removeFromDB();
    return res;
}


uiSEGYImportResult* uiSEGYReadFinisher::getImportResult( int id )
{
    return reports_.validIdx( id ) ? reports_.get( id ) : 0;
}


//uiSEGYImportResult
uiSEGYImportResult::uiSEGYImportResult( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("SEGY multi vintage import status"),
	       mNoDlgTitle, mNoHelpKey) )
{
    table_ = new uiTable( this, uiTable::Setup(0, 3), "Final Report" );
    table_->setColumnLabel( 0, uiStrings::sFile() );
    table_->setColumnLabel( 1, uiStrings::sStatus() );
    table_->setColumnLabel( 2, uiStrings::sReport() );
    table_->resizeColumnToContents( 2 );
    table_->setPrefWidthInChars( 70  );
    table_->setPrefHeightInRows( 4  );
    table_->setTableReadOnly( true );
    setCtrlStyle( uiDialog::CloseOnly );
}
