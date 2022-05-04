/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Sep 2001
________________________________________________________________________

-*/

#include "uisegyexp.h"

#include "uibatchjobdispatchersel.h"
#include "uicompoundparsel.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uifiledlg.h"
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

#include "ctxtioobj.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "oddirs.h"
#include "segybatchio.h"
#include "segydirecttr.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seisread.h"
#include "seissingtrcproc.h"
#include "seiswrite.h"
#include "settings.h"
#include "survgeom.h"
#include "zdomain.h"


class uiSEGYExpTxtHeaderDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYExpTxtHeaderDlg)
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
			mCB(this,uiSEGYExpTxtHeaderDlg,writePush) );
    wtb->attach( rightBorder );
    uiToolButton* rtb = new uiToolButton( this, "open", tr("Read file"),
			mCB(this,uiSEGYExpTxtHeaderDlg,readPush) );
    rtb->attach( leftOf, wtb );

    edfld_ = new uiTextEdit( this, "Hdr edit" );
    edfld_->setStretch( 0, 1 );
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
    FilePath fp( GetDataDir(), "Seismics" );
    uiFileDialog dlg( this, true, fp.fullPath(), uiSEGYFileSpec::fileFilter(),
			tr("Read SEG-Y Textual Header from file") );
    if ( !dlg.go() ) return;

    od_istream strm( dlg.fileName() );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open file") );
	return;
    }

    SEGY::TxtHeader txthdr;
    strm.getBin( txthdr.txt_, SegyTxtHeaderLength );
    txthdr.setAscii();
    BufferString txt; txthdr.getText( txt );
    edfld_->setText( txt );
}

void writePush( CallBacker* )
{
    FilePath fp( GetDataDir(), "Seismics" );
    uiFileDialog dlg( this,false, fp.fullPath(), nullptr,
	    tr("Write SEG-Y Textual Header to a file") );
    if ( !dlg.go() ) return;

    fp.set( dlg.fileName() );
    if ( !File::isWritable(fp.pathOnly()) )
	{ uiMSG().error(tr("Cannot write to this folder")); return; }
    const BufferString fnm( fp.fullPath() );
    if ( File::exists(fnm) && !File::isWritable(fnm) )
	{ uiMSG().error(tr("Cannot write to this file")); return; }

    if ( !edfld_->saveToFile(fnm,80,false) )
	{ uiMSG().error(tr("Failed to write to this file")); return; }
}

bool acceptOK( CallBacker* )
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
{ mODTextTranslationClass(uiSEGYExpTxtHeader)
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

BufferString getSummary() const
{
    if ( se_->autogentxthead_ )
	return BufferString( "<generate>" );
    else
	return BufferString( "<user-defined>" );
}

    uiSEGYExp*	se_;

};



uiSEGYExp::uiSEGYExp( uiParent* p, Seis::GeomType gt )
    : uiDialog(p,uiDialog::Setup(tr("Export Seismic Data to SEG-Y"),
				 mNoDlgTitle,
				 mODHelpKey(mSEGYExpHelpID)).modal(false))
    , geom_(gt)
    , autogentxthead_(true)
    , morebox_(nullptr)
    , manipbox_(nullptr)
    , batchfld_(nullptr)
    , othercrsfld_(nullptr)
    , coordsysselfld_(nullptr)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    IOObjContext ctxt( uiSeisSel::ioContext( geom_, true ) );
    uiSeisSel::Setup sssu( geom_ ); sssu.steerpol(uiSeisSel::Setup::InclSteer);
    sssu.selectcomp(true);
    seissel_ = new uiSeisSel( this, ctxt, sssu,
				    BufferStringSet(mSEGYDirectTranslNm) );
    mAttachCB( seissel_->selectionDone, uiSEGYExp::inpSel );

    uiSeisTransfer::Setup tsu( geom_ );
    tsu.withnullfill(true).fornewentry(false).onlyrange(false);
    transffld_ = new uiSeisTransfer( this, tsu );
    transffld_->attach( alignedBelow, seissel_ );

    uiObject* attachobj = transffld_->attachObj();

    if ( SI().hasProjection() )
    {
	othercrsfld_ = new uiGenInput( this, tr("Export to other CRS"),
				       BoolInpSpec(false) );
	othercrsfld_->attach( alignedBelow, transffld_ );
	mAttachCB( othercrsfld_->valuechanged, uiSEGYExp::crsCB );
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, othercrsfld_ );
	attachobj = coordsysselfld_->attachObj();
    }

    fpfld_ = new uiSEGYFilePars( this, false, nullptr, false );
    fpfld_->attach( alignedBelow, attachobj );

    txtheadfld_ = new uiSEGYExpTxtHeader( this );
    txtheadfld_->attach( alignedBelow, fpfld_ );

    const bool doebcdic =
	Settings::common().isTrue( SEGY::TxtHeader::sKeySettingEBCDIC() );
    txtheadfmtsel_ = new uiGenInput( this, tr("Text header format"),
	    BoolInpSpec(doebcdic,toUiString("EBCDIC"),uiStrings::sASCII()) );
    txtheadfmtsel_->attach( alignedBelow, txtheadfld_ );

    const bool is2d = Seis::is2D(geom_);
    const bool is2dline = geom_ == Seis::Line;
    uiSEGYFileSpec::Setup su( !is2dline );
    su.forread( false ).canbe3d( !is2d );
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfmtsel_ );

    if ( is2dline )
    {
	morebox_ = new uiCheckBox( this, uiStrings::phrExport(
				tr("more lines from the same dataset")),
				mCB(this,uiSEGYExp,showSubselCB) );
	morebox_->attach( alignedBelow, fsfld_ );
    }
    else
    {
	manipbox_ = new uiCheckBox( this,
			tr("Manipulate output file after creation") );
	manipbox_->attach( alignedBelow, fsfld_ );

	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	mAttachCB( batchfld_->checked, uiSEGYExp::batchChg );
	Batch::JobSpec& js = batchfld_->jobSpec();
	js.pars_.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyExport() );
	js.pars_.setYN( SEGY::IO::sKeyIs2D(), is2d );
	batchfld_->attach( alignedBelow,
		manipbox_ ?  manipbox_ : fsfld_->attachObj() );
    }

    mAttachCB( postFinalise(), uiSEGYExp::inpSel );
}


uiSEGYExp::~uiSEGYExp()
{
    detachAllNotifiers();
}


void uiSEGYExp::inpSel( CallBacker* )
{
    crsCB( nullptr );
    const IOObj* ioobj = seissel_->ioobj( true );
    if ( !ioobj )
	return;

    transffld_->updateFrom( *ioobj );

    const FilePath fp = ioobj->fullUserExpr();
    FilePath fnm( GetSurveyExportDir(), fp.baseName() );
    fnm.setExtension( "sgy" );
    fsfld_->setFileName( fnm.fullPath() );
}


void uiSEGYExp::crsCB( CallBacker* )
{
    if ( coordsysselfld_ )
	coordsysselfld_->display( othercrsfld_->getBoolValue() );
}


void uiSEGYExp::showSubselCB( CallBacker* )
{
    const bool multilinesel = morebox_->isChecked();
    transffld_->showSubselFld( !multilinesel );
}


void uiSEGYExp::batchChg( CallBacker* )
{
    if ( !manipbox_ || !batchfld_ )
	return;

    manipbox_->setSensitive( !batchfld_->wantBatch() );
    txtheadfld_->setSensitive( !batchfld_->wantBatch() );
}



class uiSEGYExpMore : public uiDialog
{ mODTextTranslationClass(uiSEGYExpMore)
public:

uiSEGYExpMore( uiSEGYExp* p, const IOObj& ii, const IOObj& oi )
	: uiDialog(p,uiDialog::Setup(tr("2D SEG-Y multi-export"),
				     tr("Specify file details"),
				     mODHelpKey(mSEGYExpMoreHelpID) ))
    , segyexp_(p)
    , inioobj_(ii)
    , outioobj_(oi)
{
    const BufferString fnm( outioobj_.fullUserExpr(false) );
    FilePath fp( fnm );
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
	lnmsfld_->addItem( lnms.get(idx) );
    lnmsfld_->chooseAll();

    BufferString newfnm( uiSEGYFileSpec::sKeyLineNmToken() );
    newfnm += "_"; newfnm += inioobj_.name();
    newfnm += "."; newfnm += ext;
    fp.setFileName( newfnm );
     uiString txt( tr("Output (Line name replaces '%1'")
			.arg(uiSEGYFileSpec::sKeyLineNmToken()) );

    uiFileInput::Setup fisu( fp.fullPath() );
    fisu.objtype( uiStrings::sSEGY() ).forread( false );
    fnmfld_ = new uiFileInput( this, txt, fisu );
    fnmfld_->attach( alignedBelow, lnmsfld_ );
}


bool acceptOK( CallBacker* )
{
    BufferString fnm = fnmfld_->fileName();
    FilePath fp( fnm );
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

    IOM().to( inioobj_.key() );
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

bool doExp( const FilePath& fp )
{
    BufferStringSet lnms;
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( lnmsfld_->isChosen(idx) )
	    lnms.add( lnmsfld_->textOfItem(idx) );
    }
    if ( lnms.size() < 1 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("lines to export")) );
	return false;
    }

    bool nofails = true;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const BufferString& lnm = *lnms[idx];
	BufferString filenm( fp.fullPath() );
	filenm.replace( uiSEGYFileSpec::sKeyLineNmToken(), lnm );
	IOObj* newioobj = getSubstIOObj( filenm );
	if ( !doWork( newioobj, lnm, idx > lnms.size()-2, nofails ) )
	    return false;
    }

    return nofails;
}

    uiFileInput*	fnmfld_;
    uiListBox*		lnmsfld_;
    uiSEGYExp*		segyexp_;

    const IOObj&	inioobj_;
    const IOObj&	outioobj_;

};


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSEGYExp::acceptOK( CallBacker* )
{
    const IOObj* inioobj = seissel_->ioobj(true);
    if ( !inioobj )
	mErrRet( uiStrings::phrSelect(tr("the data to export")) )
    const SEGY::FileSpec sfs( fsfld_->getSpec() );
    if ( sfs.isEmpty() )
	mErrRet( uiStrings::phrSelect(uiStrings::sOutputFile().toLower()) )

    PtrMan<IOObj> outioobj = sfs.getIOObj( true );

    const bool usecrs = othercrsfld_ && othercrsfld_->getBoolValue();
    if ( usecrs )
    {
	SEGY::FilePars filepars = fpfld_->getPars();
	filepars.setCoordSys( coordsysselfld_->getCoordSystem() );
	fpfld_->setPars( filepars );
    }

    fpfld_->fillPar( outioobj->pars() );
    const bool is2d = Seis::is2D( geom_ );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIs2D(), is2d );
    outioobj->pars().setYN( SeisTrcTranslator::sKeyIsPS(), Seis::isPS(geom_) );

    const bool doebcdic = txtheadfmtsel_->getBoolValue();
    Settings::common().setYN( SEGY::TxtHeader::sKeySettingEBCDIC(), doebcdic );
    Settings::common().write();

    if ( batchfld_ && batchfld_->wantBatch() )
    {
	batchfld_->setJobName( fsfld_->getJobNameFromFileName() );
	Batch::JobSpec& js = batchfld_->jobSpec();
	IOPar inpars;
	seissel_->fillPar( inpars );
	js.pars_.mergeComp( inpars, sKey::Input() );

	IOPar outpars;
	transffld_->fillPar( outpars );
	fpfld_->fillPar( outpars );
	fsfld_->fillPar( outpars );
	if ( usecrs )
	    coordsysselfld_->getCoordSystem()->fillPar( outpars );

// TODO: Support header text
	js.pars_.mergeComp( outpars, sKey::Output() );
	batchfld_->start();
	return false;
    }

    const bool multilinesel = morebox_ && morebox_->isChecked();
    const char* lnm = is2d && !multilinesel && transffld_->selFld2D()
			   && transffld_->selFld2D()->isSingLine()
		    ? transffld_->selFld2D()->selectedLine() : nullptr;
    bool needmsgallok = false;
    if ( multilinesel )
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
	    uiSEGYFileManip dlg( this, outioobj->fullUserExpr(false) );
	    dlg.go();
	}
    }

    if ( needmsgallok )
	uiMSG().message( tr("Successful export of:\n%1").arg(sfs.dispName()) );

    return false;
}


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj,
			const char* linenm )
{
    const bool is2d = Seis::is2D( geom_ );
    PtrMan<uiSeisIOObjInfo> ioobjinfo = new uiSeisIOObjInfo( outioobj, true );
    if ( !ioobjinfo->checkSpaceLeft(transffld_->spaceInfo()) )
	return false;

    const IOObj* useoutioobj = &outioobj;
    IOObj* tmpioobj = nullptr;
    const bool inissidom = ZDomain::isSI( inioobj.pars() );
    if ( !inissidom )
    {
	tmpioobj = outioobj.clone();
	ZDomain::Def::get(inioobj.pars()).set( tmpioobj->pars() );
	useoutioobj = tmpioobj;
    }

    SEGY::TxtHeader::info2D() = is2d;
    SEGY::TxtHeader::isPS() = Seis::isPS( geom_ );
#   define mRet(yn) \
    { delete tmpioobj; \
      SEGY::TxtHeader::info2D() = false; \
      SEGY::TxtHeader::isPS() = false; \
      return yn; }

    PtrMan<Executor> exec = transffld_->getTrcProc( inioobj, *useoutioobj,
				    "Output seismic data", tr("Writing traces"),
				    linenm );
    if ( !exec )
	mRet( false )

    mDynamicCastGet(SeisSingleTraceProc*,sstp,exec.ptr())
    if ( sstp )
    {
	if ( !sstp->reader(0) )
	    mRet( false )

	SeisTrcReader& rdr = const_cast<SeisTrcReader&>( *sstp->reader(0) );
	SeisIOObjInfo oinf( rdr.ioObj() );
	rdr.setComponent( seissel_->compNr() );

	const SeisTrcWriter& wrr = sstp->writer();
	SeisTrcTranslator* transl =
			const_cast<SeisTrcTranslator*>(wrr.seisTranslator());
	mDynamicCastGet(SEGYSeisTrcTranslator*,segytr,transl)
	const bool usecrs = othercrsfld_ && othercrsfld_->getBoolValue();
	if ( segytr && usecrs )
	    segytr->setCoordSys( coordsysselfld_->getCoordSystem() );

	if ( !autogentxthead_ && !hdrtxt_.isEmpty() && segytr )
	{
	    SEGY::TxtHeader* th = new SEGY::TxtHeader;
	    th->setText( hdrtxt_ );
	    segytr->setTxtHeader( th );
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
    if ( tmpioobj )
	IOM().commitChanges( *tmpioobj );

    mRet( rv )
}
