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
#include "oddirs.h"
#include "od_helpids.h"
#include "segybatchio.h"
#include "segydirecttr.h"
#include "segyhdr.h"
#include "segytr.h"
#include "seisread.h"
#include "seissingtrcproc.h"
#include "seiswrite.h"
#include "od_istream.h"
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
    sssu.trsnotallwed_.add( mSEGYDirectTranslNm );
    seissel_ = new uiSeisSel( this, ctxt, sssu );
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

    const bool is2d = Seis::is2D( geom_ );
    const bool issingle2dline = geom_ == Seis::Line;
    uiSEGYFileSpec::Setup su( !issingle2dline );
    su.forread( false ).canbe3d( !is2d );
    fsfld_ = new uiSEGYFileSpec( this, su );
    fsfld_->attach( alignedBelow, txtheadfld_ );

    if ( issingle2dline )
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
	batchfld_->attach( alignedBelow,
			   manipbox_ ?	manipbox_ : fsfld_->attachObj() );
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
    const uiString txt( tr("Output (Line name replaces '%1'")
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


bool doWork( IOObj* newioobj, bool islast, bool& nofails )
{
    const IOObj& in = inioobj_; const IOObj& out = *newioobj;
    bool res = segyexp_->doWork( in, out );
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

	uiSeis2DSubSel* seissel2d = segyexp_->transffld_->selFld2D();
	if ( seissel2d && seissel2d->isSingLine() )
	    seissel2d->setSelectedLine( lnm );

	if ( !doWork(newioobj,idx>lnms.size()-2,nofails) )
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

    if ( !autogentxthead_ && !hdrtxt_.isEmpty() )
	transffld_->setOutputHeader( hdrtxt_ );

    if ( othercrsfld_ && othercrsfld_->getBoolValue() )
    {
	ConstRefMan<Coords::CoordSystem> crs =
					 coordsysselfld_->getCoordSystem();
	if ( crs )
	{
	    transffld_->setCoordSystem( *crs.ptr(), false );
	    SEGY::FilePars filepars = fpfld_->getPars();
	    filepars.setCoordSys( crs );
	    fpfld_->setPars( filepars );
	}
    }

    if ( batchfld_ && batchfld_->wantBatch() )
    {
	const BufferString jobname( "Export_SEG-Y_",
				    fsfld_->getJobNameFromFileName() );
	batchfld_->setJobName( jobname );
	IOPar& jobpars = batchfld_->jobSpec().pars_;
	jobpars.setEmpty();
	Seis::putInPar( geom_, jobpars );
	jobpars.set( SEGY::IO::sKeyTask(), SEGY::IO::sKeyExport() );

	IOPar inpars;
	seissel_->fillPar( inpars );
	jobpars.mergeComp( inpars, sKey::Input() );

	IOPar outpars;
	transffld_->fillPar( outpars );
	fpfld_->fillPar( outpars );
	fsfld_->fillPar( outpars );

	jobpars.mergeComp( outpars, sKey::Output() );
	batchfld_->start();
	return false;
    }

    PtrMan<IOObj> outioobj = sfs.getIOObj( true );
    fpfld_->fillPar( outioobj->pars() );

    bool needmsgallok = false;
    const bool multilinesel = morebox_ && morebox_->isChecked();
    if ( multilinesel )
    {
	uiSEGYExpMore dlg( this, *inioobj, *outioobj );
	dlg.go();
    }
    else
    {
	const bool result = doWork( *inioobj, *outioobj );
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


bool uiSEGYExp::doWork( const IOObj& inioobj, const IOObj& outioobj )
{
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

#   define mRet(yn) \
    { delete tmpioobj; return yn; }

    BufferString execnm( "Output seismic data" );
    if ( transffld_->selFld2D() && transffld_->selFld2D()->isSingLine() )
    {
	execnm.add( " (" ).add( transffld_->selFld2D()->selectedLine() )
	      .add( ")" );
    }

    PtrMan<Executor> exec = transffld_->getTrcProc( inioobj, *useoutioobj,
			    execnm, tr("Writing traces"),
			    seissel_->compNr() );
    if ( !exec )
	mRet( false )

    uiTaskRunner dlg( this );
    const bool rv = TaskRunner::execute( &dlg, *exec );
    if ( tmpioobj )
	IOM().commitChanges( *tmpioobj );

    mRet( rv )
}
