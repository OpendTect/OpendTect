/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadfinisher.h"
#include "uiseissel.h"
#include "uiwellsel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "welltransl.h"
#include "wellman.h"
#include "segybatchio.h"
#include "segydirecttr.h"
#include "segydirectdef.h"
#include "segyscanner.h"
#include "seistrc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisimporter.h"
#include "seisioobjinfo.h"
#include "segytr.h"
#include "welldata.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "wellman.h"
#include "welllog.h"
#include "welltrack.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "ioobj.h"
#include "ioman.h"
#include "filepath.h"


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
    uiString ret( "Importing %1" );
    ret.arg( usrspec );
    return ret;
}


uiSEGYReadFinisher::uiSEGYReadFinisher( uiParent* p, const FullSpec& fs,
					const char* usrspec )
    : uiDialog(p,uiDialog::Setup(getWinTile(fs),getDlgTitle(usrspec),
				  mTODOHelpKey ) )
    , fs_(fs)
    , outwllfld_(0)
    , lognmfld_(0)
    , inpdomfld_(0)
    , isfeetfld_(0)
    , outimpfld_(0)
    , outscanfld_(0)
    , transffld_(0)
    , batchfld_(0)
    , docopyfld_(0)
{
    objname_ = FilePath( usrspec ).baseName();
    const bool is2d = Seis::is2D( fs_.geomType() );
    if ( !is2d )
	objname_.replace( '*', 'x' );

    if ( fs_.isVSP() )
	crVSPFields();
    else
	crSeisFields();

    postFinalise().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


void uiSEGYReadFinisher::crSeisFields()
{
    const Seis::GeomType gt = fs_.geomType();
    const bool is2d = Seis::is2D( gt );

    if ( gt != Seis::Line )
    {
	docopyfld_ = new uiGenInput( this, "Copy data",
		BoolInpSpec(true,tr("Yes (import)"),tr("No (scan&&link)")) );
	docopyfld_->valuechanged.notify(mCB(this,uiSEGYReadFinisher,doScanChg));
    }

    uiSeisTransfer::Setup trsu( gt );
    trsu.withnullfill( false ).fornewentry( true );
    transffld_ = new uiSeisTransfer( this, trsu );
    if ( docopyfld_ )
	transffld_->attach( alignedBelow, docopyfld_ );
    if ( is2d )
	transffld_->selFld2D()->setSelectedLine( objname_ );

    uiSeisSel::Setup copysu( gt ); copysu.enabotherdomain( true );
    IOObjContext ctxt( uiSeisSel::ioContext( gt, false ) );
    outimpfld_ = new uiSeisSel( this, ctxt, copysu );
    outimpfld_->attach( alignedBelow, transffld_ );
    if ( !is2d )
	outimpfld_->setInputText( objname_ );

    if ( gt != Seis::Line )
    {
	uiSeisSel::Setup scansu( gt );
	scansu.enabotherdomain( true ).withwriteopts( false );
	ctxt.toselect.allownonuserselectable_ = true;
	ctxt.fixTranslator( SEGYDirectSeisTrcTranslator::translKey() );
	outscanfld_ = new uiSeisSel( this, ctxt, scansu );
	outscanfld_->attach( alignedBelow, transffld_ );
	if ( !is2d )
	    outscanfld_->setInputText( objname_ );

	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	batchfld_->setJobName( "Read SEG-Y" );
	batchfld_->jobSpec().pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(gt));
	batchfld_->attach( alignedBelow, outimpfld_ );
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
    inpdomfld_->valuechanged.notify( mCB(this,uiSEGYReadFinisher,inpDomChg) );
    isfeetfld_ = new uiCheckBox( this, "in Feet" );
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


void uiSEGYReadFinisher::doScanChg( CallBacker* )
{
    if ( !docopyfld_ )
	return;

    const bool copy = docopyfld_->getBoolValue();
    outimpfld_->display( copy );
    transffld_->display( copy );
    if ( outscanfld_ )
	outscanfld_->display( !copy );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSEGYReadFinisher::doVSP()
{
    const IOObj* wllioobj = outwllfld_->ioobj();
    if ( !wllioobj )
	return false;
    const BufferString lognm( lognmfld_->text() );
    if ( lognm.isEmpty() )
	mErrRet(tr("Please enter a valid name for the new log"))

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
	    z = wd->d2TModel()->getDah( z, track );
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


SeisStdImporterReader* uiSEGYReadFinisher::getImpReader( const IOObj& ioobj )
{
    SeisStdImporterReader* rdr = new SeisStdImporterReader( ioobj, "SEG-Y" );
    rdr->removeNull( transffld_->removeNull() );
    rdr->setResampler( transffld_->getResampler() );
    rdr->setScaler( transffld_->getScaler() );
    Seis::SelData* sd = transffld_->getSelData();
    if ( sd )
	rdr->setSelData( sd );
    return rdr;
}


bool uiSEGYReadFinisher::do3D( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp )
{
    Executor* exec;
    const Seis::GeomType gt = fs_.geomType();
    PtrMan<SeisTrcWriter> wrr; PtrMan<SeisImporter> imp;
    PtrMan<SEGY::FileIndexer> indexer;
    if ( doimp )
    {
	wrr = new SeisTrcWriter( &outioobj );
	imp = new SeisImporter( getImpReader(inioobj), *wrr, gt );
	exec = imp.ptr();
    }
    else
    {
	indexer = new SEGY::FileIndexer( outioobj.key(), !Seis::isPS(gt),
			fs_.spec_, false, inioobj.pars() );
	exec = indexer.ptr();
    }

    uiTaskRunner dlg( this );
    if ( !dlg.execute( *exec ) )
	return false;

    BufferStringSet warns;
    if ( indexer )
	warns.add( indexer->scanner()->warnings(), false );
    else
    {
	if ( imp->nrSkipped() > 0 )
	    warns += new BufferString("During import, ", imp->nrSkipped(),
				      " traces were rejected" );
	SeisStdImporterReader& stdrdr
		= static_cast<SeisStdImporterReader&>( imp->reader() );
	SeisTrcTranslator* transl = stdrdr.reader().seisTranslator();
	if ( transl && transl->haveWarnings() )
	    warns.add( transl->warnings(), false );
    }
    imp.erase(); wrr.erase(); // closes output cube

    if ( !uiSEGY::displayWarnings(warns,doimp) )
    {
	IOM().permRemove( outioobj.key() );
	return false;
    }

    if ( indexer )
    {
	IOPar rep( "SEG-Y scan report" ); indexer->scanner()->getReport( rep );
	uiSEGY::displayReport( parent(), rep );
    }
    return true;
}


bool uiSEGYReadFinisher::do2D( const IOObj& inioobj, const IOObj& outioobj,
				bool doimp )
{
    uiMSG().error( "TODO: 2D non-batch import" );
    return false;
}


bool uiSEGYReadFinisher::doBatch( bool doimp )
{
    batchfld_->setJobName( doimp ? "import SEG-Y" : "scan SEG-Y" );
    IOPar& jobpars = batchfld_->jobSpec().pars_;
    const bool isps = Seis::isPS( fs_.geomType() );
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
	    : (isps ? SEGY::IO::sKeyIndexPS() : SEGY::IO::sKeyIndex3DVol()) );
    fs_.fillPar( jobpars );

    IOPar outpars;
    transffld_->fillPar( outpars );
    outFld(doimp)->fillPar( outpars );
    jobpars.mergeComp( outpars, sKey::Output() );

    return batchfld_->start();
}


bool uiSEGYReadFinisher::acceptOK( CallBacker* )
{
    if ( fs_.isVSP() )
	return doVSP();

    const bool doimp = docopyfld_ ? docopyfld_->getBoolValue() : true;
    const IOObj* outioobj = outFld(doimp)->ioobj();
    if ( !outioobj )
	return false;

    const bool dobatch = batchfld_ && batchfld_->wantBatch();
    const bool is2d = Seis::is2D( fs_.geomType() );
    const bool isps = Seis::isPS( fs_.geomType() );

    PtrMan<uiSeisIOObjInfo> ioobjinfo;
    if ( doimp && !isps )
    {
	ioobjinfo = new uiSeisIOObjInfo( *outioobj, true );
	if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	    return false;
    }

    if ( dobatch )
	return doBatch( doimp );

    PtrMan<IOObj> inioobj = fs_.spec_.getIOObj( true );
    const bool outissidom = ZDomain::isSI( outioobj->pars() );
    if ( !outissidom )
	ZDomain::Def::get(outioobj->pars()).set( inioobj->pars() );
    IOM().commitChanges( *inioobj );

    bool isok = is2d ? do2D( *inioobj, *outioobj, doimp )
		     : do3D( *inioobj, *outioobj, doimp );
    if ( !isok )
	return false;

    if ( !is2d && ioobjinfo )
	 isok = ioobjinfo->provideUserInfo();

    return isok;
}
