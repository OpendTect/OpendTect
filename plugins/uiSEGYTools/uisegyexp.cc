/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2001
________________________________________________________________________

-*/

#include "uisegyexp.h"

#include "uibatchjobdispatchersel.h"
#include "uicompoundparsel.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisegydef.h"
#include "uisegymanip.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ioobjctxt.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iostrm.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "segybatchio.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seisprovider.h"
#include "seissingtrcproc.h"
#include "seisstorer.h"
#include "od_istream.h"
#include "survgeom.h"
#include "zdomain.h"

class uiSEGYExpTxtHeaderDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYExpTxtHeaderDlg);
public:

uiSEGYExpTxtHeaderDlg( uiParent* p, BufferString& hdr, bool& ag )
    : uiDialog(p,Setup(tr("Define SEG-Y Text Header"),uiSEGYExp::sTxtHeadTxt(),
                        mODHelpKey(mSEGYExpTxtHeaderDlgHelpID) ))
    , hdr_(hdr)
    , autogen_(ag)
{
    const CallBack cb( mCB(this,uiSEGYExpTxtHeaderDlg,agSel) );
    autogenfld_ = new uiGenInput( this, tr("Automatically generate"),
				  BoolInpSpec(false) );
    autogenfld_->valuechanged.notify( cb );
    uiToolButton* wtb = new uiToolButton( this, "save", tr("Write to file"),
			    mCB(this,uiSEGYExpTxtHeaderDlg,writePush));
    wtb->attach( rightBorder );
    uiToolButton* rtb = new uiToolButton( this, "open", tr("Read file"),
			    mCB(this,uiSEGYExpTxtHeaderDlg,readPush) );
    rtb->attach( leftOf, wtb );

    edfld_ = new uiTextEdit( this, "Hdr edit" );
    edfld_->setPrefWidthInChar( 81 );
    edfld_->setPrefHeightInChar( 24 );
    if ( hdr_.isEmpty() )
    {
	SEGY::TxtHeader th; th.clear();
	th.getText( hdr_ );
    }
    edfld_->setText( hdr_ );
    edfld_->attach( ensureBelow, autogenfld_ );
    postFinalise().notify( cb );
}

void agSel( CallBacker* )
{
    edfld_->display( !autogenfld_->getBoolValue() );
}

void readPush( CallBacker* )
{
    File::Path fp( GetDataDir(), sSeismicSubDir() );
    uiFileSelector::Setup fssu;
    fssu.initialselectiondir( fp.fullPath() )
	.formats( uiSEGYFileSpec::fileFmts() );
    uiFileSelector uifs( this, fssu );
    uifs.caption() = tr("Read SEG-Y Textual Header from file");
    if ( !uifs.go() )
	return;

    od_istream strm( uifs.fileName() );
    if ( !strm.isOK() )
	{ uiMSG().error(tr("Cannot open file")); return; }

    SEGY::TxtHeader txthdr;
    strm.getBin( txthdr.txt_, SegyTxtHeaderLength );
    txthdr.setAscii();
    BufferString txt; txthdr.getText( txt );
    edfld_->setText( txt );
}

void writePush( CallBacker* )
{
    File::Path fp( GetDataDir(), sSeismicSubDir() );
    uiFileSelector::Setup fssu;
    fssu.setForWrite()
	.initialselectiondir( fp.fullPath() )
	.formats( uiSEGYFileSpec::fileFmts() );
    uiFileSelector uifs( this, fssu );
    uifs.caption() = tr("Write SEG-Y Textual Header to a file");
    if ( !uifs.go() )
	return;

    fp.set( uifs.fileName() );
    if ( !File::isWritable(fp.pathOnly()) )
	{ uiMSG().error(tr("Cannot write to this directory")); return; }
    const BufferString fnm( fp.fullPath() );
    if ( File::exists(fnm) && !File::isWritable(fnm) )
	{ uiMSG().error(tr("Cannot write to this file")); return; }

    if ( !edfld_->saveToFile(fnm,80,false) )
	{ uiMSG().error(tr("Failed to write to this file")); return; }
}

bool acceptOK()
{
    autogen_ = autogenfld_->getBoolValue();
    if ( !autogen_ )
	hdr_ = edfld_->text();
    return true;
}

    BufferString&	hdr_;
    bool&		autogen_;
    const uiString	fdobjtyp_;

    uiGenInput*		autogenfld_;
    uiTextEdit*		edfld_;

};

class uiSEGYExpTxtHeader : public uiCompoundParSel
{ mODTextTranslationClass(uiSEGYExpTxtHeader);
public:

uiSEGYExpTxtHeader( uiSEGYExp* se )
    : uiCompoundParSel(se,tr("Text header"),OD::Define)
    , se_(se)
{
    butPush.notify( mCB(this,uiSEGYExpTxtHeader,butPushed) );
}

void butPushed( CallBacker* )
{
    uiSEGYExpTxtHeaderDlg dlg( this, se_->hdrtxt_, se_->autogentxthead_ );
    dlg.go();
}

uiString getSummary() const
{
    if ( se_->autogentxthead_ )
	return uiStrings::sGenerate().toLower().embedFinalState();
    else
	return uiStrings::sUserDefined().toLower().embedFinalState();
}

    uiSEGYExp*	se_;

};



uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
	: uiDialog(p,uiDialog::Setup(tr("SEG-Y I/O"),
				     uiStrings::phrExport(tr("to SEG-Y")),
                                     mODHelpKey(mSEGYExpHelpID) ))
	, geom_(gt)
	, morebox_(0)
	, manipbox_(0)
	, batchfld_(0)
	, autogentxthead_(true)
{
    setCtrlStyle( RunAndClose );
    const CallBack inpselcb( mCB(this,uiSEGYExp,inpSel) );

    IOObjContext ctxt( uiSeisSel::ioContext( geom_, true ) );
    uiSeisSel::Setup sssu( geom_ ); sssu.steerpol( Seis::InclSteer );
    sssu.selectcomp( true );
    seissel_ = new uiSeisSel( this, ctxt, sssu );
    seissel_->selectionDone.notify( inpselcb );

    uiSeisTransfer::Setup tsu( geom_ );
    tsu.withnullfill(true).fornewentry(false).onlyrange(false);
    transffld_ = new uiSeisTransfer( this, tsu );
    transffld_->attach( alignedBelow, seissel_ );

    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
    coordsysselfld_->attach( alignedBelow, transffld_ );

    fpfld_ = new uiSEGYFilePars( this, false, 0, false );
    fpfld_->attach( alignedBelow, coordsysselfld_ );

    txtheadfld_ = new uiSEGYExpTxtHeader( this );
    txtheadfld_->attach( alignedBelow, fpfld_ );

    const bool is2d = Seis::is2D(geom_);
    const bool is2dline = geom_ == Seis::Line;
    uiSEGYFileSpec::Setup su( !is2dline );
    su.forread( false ).canbe3d( !is2d );
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfld_ );

    if ( is2dline )
    {
	morebox_ = new uiCheckBox( this, uiStrings::phrExport(
				    tr("more lines from the same dataset")) );
	morebox_->attach( alignedBelow, fsfld_ );
    }
    else
    {
	manipbox_ = new uiCheckBox( this,
			tr("Manipulate output file after creation") );
	manipbox_->attach( alignedBelow, fsfld_ );

	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	batchfld_->checked.notify( mCB(this,uiSEGYExp,batchChg) );
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyExport() );
	js.pars_.setYN( SEGY::IO::sKeyIs2D(), is2d );
	batchfld_->attach( alignedBelow,
		manipbox_ ?  manipbox_ : fsfld_->attachObj() );
    }

    postFinalise().notify( inpselcb );
}


void uiSEGYExp::inpSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj(true);
    if ( ioobj )
	transffld_->updateFrom( *ioobj );
}


void uiSEGYExp::batchChg( CallBacker* )
{
    if ( !manipbox_ || !batchfld_ )
	return;

    manipbox_->setSensitive( !batchfld_->wantBatch() );
    txtheadfld_->setSensitive( !batchfld_->wantBatch() );
}



class uiSEGYExpMore : public uiDialog
{ mODTextTranslationClass(uiSEGYExpMore);
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi )
	: uiDialog(p,uiDialog::Setup(tr("2D SEG-Y multi-export"),
				     tr("Specify file details"),
                                     mODHelpKey(mSEGYExpMoreHelpID) ))
	, inioobj_(ii)
	, outioobj_(oi)
	, segyexp_(p)
{
    const BufferString fnm( outioobj_.mainFileName() );
    File::Path fp( fnm );
    BufferString ext = fp.extension();
    if ( ext.isEmpty() ) ext = "sgy";
    BufferString setupnm( "Exp " );
    setupnm += uiSEGYFileSpec::sKeyLineNmToken();

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Lines to export") );
    lnmsfld_ = new uiListBox( this, su );
    SeisIOObjInfo sii( inioobj_ );
    BufferStringSet lnms;
    sii.getLineNames( lnms );
    for ( int idx=0; idx<lnms.size(); idx++ )
	lnmsfld_->addItem( toUiString(lnms.get(idx)) );
    lnmsfld_->chooseAll();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "_"; newfnm += inioobj_.name();
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
    uiString txt( tr("Output (Line name replaces '%1'")
				    .arg(uiSEGYFileSpec::sKeyLineNmToken()) );

    uiFileSel::Setup fssu( fp.fullPath() );
    fssu.objtype( uiStrings::sSEGY() ).setForWrite();
    fnmfld_ = new uiFileSel( this, txt, fssu );
    fnmfld_->attach( alignedBelow, lnmsfld_ );
}


bool acceptOK()
{
    BufferString fnm = fnmfld_->fileName();
    File::Path fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    if ( !File::isDirectory(dirnm) )
	File::createDir( dirnm );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
    {
	uiMSG().error( tr("Directory provided not usable") );
	return false;
    }
    fnm = fp.fullPath();
    if ( !fnm.contains(uiSEGYFileSpec::sKeyLineNmToken()) )
    {
	uiString msg = tr("The file name has to contain at least one '%1'\n"
			  "That will then be replaced by the line name")
		     .arg(uiSEGYFileSpec::sKeyLineNmToken());
	uiMSG().error( msg );
	return false;
    }

    return doExp( fp );
}


IOObj* getSubstIOObj( const char* fullfnm )
{
    IOObj* newioobj = outioobj_.clone();
    newioobj->setName( fullfnm );
    mDynamicCastGet(IOStream*,iostrm,newioobj)
    iostrm->fileSpec().setFileName( fullfnm );
    return newioobj;
}


bool doWork( IOObj* newioobj, const char* lnm, bool islast, bool& nofails )
{
    const IOObj& in = inioobj_; const IOObj& out = *newioobj;
    bool res = segyexp_->doWork( in, out, lnm );
    delete newioobj;
    if ( !res )
    {
	nofails = false;
	if ( !islast && !uiMSG().askContinue(tr("Continue with next?")) )
	    return false;
    }
    return true;
}

bool doExp( const File::Path& fp )
{
    BufferStringSet lnms;
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( lnmsfld_->isChosen(idx) )
	    lnms.add( lnmsfld_->itemText(idx) );
    }
    if ( lnms.size() < 1 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("lines to export")) );
	return false;
    }

    bool nofails = true;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString& lnm = lnms.get( idx );
	BufferString filenm( fp.fullPath() );
	filenm.replace( uiSEGYFileSpec::sKeyLineNmToken(), lnm );
	IOObj* newioobj = getSubstIOObj( filenm );
	if ( !doWork( newioobj, lnm, idx > lnms.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileSel*		fnmfld_;
    uiListBox*		lnmsfld_;
    uiSEGYExp*		segyexp_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;

};


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK()
{
    const IOObj* inioobj = seissel_->ioobj(true);
    if ( !inioobj )
	mErrRet( uiStrings::phrSelect(tr("the data to export")) )
    const SEGY::FileSpec sfs( fsfld_->getSpec() );
    if ( sfs.isEmpty() )
	mErrRet( uiStrings::phrSelect(uiStrings::sOutputFile().toLower()) )

    PtrMan<IOObj> outioobj = sfs.getIOObj( true );

    SEGY::FilePars filepars = fpfld_->getPars();
    filepars.setCoordSys( coordsysselfld_->getCoordSystem() );
    fpfld_->setPars( filepars );

    fpfld_->fillPar( outioobj->pars() );

    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( sKey::IsPS(), Seis::isPS(geom_) );

    if ( batchfld_ && batchfld_->wantBatch() )
    {
	batchfld_->setJobName( fsfld_->getJobNameForBatchProcess() );
	Batch::JobSpec& js = batchfld_->jobSpec();
	IOPar inpars;
	seissel_->fillPar( inpars );
	js.pars_.mergeComp( inpars, sKey::Input() );

	IOPar outpars;
	transffld_->fillPar( outpars );
	fpfld_->fillPar( outpars );
	fsfld_->fillPar( outpars );
// TODO: Support header text
	js.pars_.mergeComp( outpars, sKey::Output() );
	batchfld_->start();
	return false;
    }

    const char* lnm = is2d && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : 0;
    bool needmsgallok = false;
    if ( morebox_ && morebox_->isChecked() )
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj );
	dlg.go();
    }
    else
    {
	bool result = doWork( *inioobj, *outioobj, lnm );
	if ( !result || !manipbox_ || !manipbox_->isChecked() )
	    needmsgallok = result;
	else
	{
	    uiSEGYFileManip dlg( this, outioobj->mainFileName() );
	    dlg.go();
	}
    }

    if ( needmsgallok )
	uiMSG().message( tr("Successful export of:\n%1").arg(sfs.dispName()) );

    raise();
    return false;
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
			const char* linenm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( this, outioobj );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo(),true) )
	return false;

    const IOObj* useoutioobj = &outioobj;
    IOObj* tmpioobj = 0;
    const bool inissidom = ZDomain::isSI( inioobj.pars() );
    if ( !inissidom )
    {
	tmpioobj = outioobj.clone();
	ZDomain::Def::get(inioobj.pars()).set( tmpioobj->pars() );
	useoutioobj = tmpioobj;
    }

    SEGY::TxtHeader::info2D() = is2d;
#   define mRet(yn) \
    { delete tmpioobj; SEGY::TxtHeader::info2D() = false; return yn; }

    Executor* exec = transffld_->getTrcProc( inioobj, *useoutioobj,
				    "Output seismic data", tr("Writing traces"),
				    linenm );
    if ( !exec )
	mRet( false )
    PtrMan<Executor> execptrman = exec;

    mDynamicCastGet(SeisSingleTraceProc*,sstp,exec)
    if ( sstp )
    {
	if ( !sstp->provider() )
	    mRet( false )
	Seis::Provider& prov =
		const_cast<Seis::Provider&>( *sstp->provider() );
	prov.selectComponent( seissel_->compNr() );

	if ( !autogentxthead_ && !hdrtxt_.isEmpty() )
	{
	    auto& storer = mNonConst( sstp->storer() );
	    auto* transl = const_cast<SeisTrcTranslator*>(storer.translator());
	    mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,transl)
	    if ( segytr )
	    {
		segytr->setCoordSys( coordsysselfld_->getCoordSystem() );
		SEGY::TxtHeader* th = new SEGY::TxtHeader;
		th->setText( hdrtxt_ );
		segytr->setTxtHeader( th );
	    }
	}
    }

    bool rv = false;
    if ( linenm && *linenm )
    {
	BufferString nm( exec->name() );
	nm += " ("; nm += linenm; nm += ")";
	exec->setName( nm );
    }

    uiTaskRunner dlg( this );
    rv = TaskRunner::execute( &dlg, *exec );
    execptrman.erase();

    if ( tmpioobj )
	tmpioobj->commitChanges();

    mRet( rv )
}
