/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegyread.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uifileinput.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uisegydef.h"
#include "uisegydefdlg.h"
#include "uisegyexamine.h"
#include "uisegyimpdlg.h"
#include "uisegyscandlg.h"
#include "uitaskrunner.h"
#include "uivarwizarddlg.h"

#include "ctxtioobj.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "segyscanner.h"
#include "segyfiledata.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "timefun.h"


static const char* sKeySEGYRev1Pol = "SEG-Y Rev. 1 policy";

#define mSetState(st) mSetVWState( ((int)(st)) )


void uiSEGYRead::Setup::getDefaultTypes( TypeSet<Seis::GeomType>& geoms,
					 bool forsisetup )
{
    if ( forsisetup || SI().has3D() )
    {
	geoms += Seis::Vol;
	geoms += Seis::VolPS;
    }
    if ( forsisetup || SI().has2D() )
    {
	geoms += Seis::Line;
	geoms += Seis::LinePS;
    }
}


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su,
			const IOPar* iop )
    : uiVarWizard(p)
    , setup_(su)
    , geom_(SI().has3D()?Seis::Vol:Seis::Line)
    , scanner_(0)
    , rev_(Rev0)
    , revpolnr_(2)
    , defdlg_(0)
    , examdlg_(0)
    , impdlg_(0)
    , scandlg_(0)
    , rev1qdlg_(0)
{
    if ( iop )
	usePar( *iop );

    state_ = (int)su.initialstate_;
    nextAction();
}


uiSEGYRead::~uiSEGYRead()
{
    if ( examdlg_ )
    {
        examdlg_->objectToBeDeleted().remove(mCB(this,uiSEGYRead,examDlgClose));
        delete examdlg_;
    }

    delete scanner_;
}


void uiSEGYRead::closeDown()
{
    uiVarWizard::closeDown();
}


void uiSEGYRead::doPart()
{
    switch ( state_ )
    {
    case BasicOpts:
	getBasicOpts();
    break;
    case SetupImport:
	setupImport();
    break;
    case SetupScan:
	setupScan();
    break;
    }
}


void uiSEGYRead::setGeomType( const IOObj& ioobj )
{
    bool is2d = false; bool isps = false;
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIs2D(), is2d );
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );
    geom_ = Seis::geomTypeOf( is2d, isps );
}


void uiSEGYRead::use( const IOObj* ioobj, bool force )
{
    if ( !ioobj ) return;

    FileReadOpts::shallowClear( pars_ );
    pars_.merge( ioobj->pars() );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
	setGeomType( *ioobj );
}


void uiSEGYRead::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    if ( rev_ == Rev0 )
	iop.setYN( FilePars::sKeyForceRev0(), true );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    FileReadOpts::shallowClear( pars_ );
    pars_.merge( iop );
    rev_ = iop.isTrue( FilePars::sKeyForceRev0() ) ? Rev0 : Rev1;
}


void uiSEGYRead::writeReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    mDynamicCastGet(uiSEGYDefDlg*,defdlg,cb)
    if ( !rddlg && !defdlg )
	{ pErrMsg("Huh"); return; }

    uiParent* parnt = rddlg;
    if ( !parnt ) parnt = defdlg;
    PtrMan<CtxtIOObj> ctio = getCtio( false );
    uiIOObjSelDlg dlg( parnt, *ctio, tr("New SEG-Y setup") );
    dlg.setModal( true );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    if ( rddlg )
	rddlg->updatePars();
    else
	defdlg->fillPar( pars_ );

    fillPar( ioobj->pars() );
    ioobj->pars().removeWithKey( uiSEGYExamine::Setup::sKeyNrTrcs );
    ioobj->pars().removeWithKey( sKey::Geometry() );
    IOM().commitChanges( *ioobj );
}


void uiSEGYRead::readReq( CallBacker* cb )
{
    uiDialog* parnt = defdlg_;
    uiSEGYReadDlg* rddlg = 0;
    if ( parnt )
	geom_ = defdlg_->geomType();
    else
    {
	mDynamicCastGet(uiSEGYReadDlg*,dlg,cb)
	if ( !dlg ) { pErrMsg("Huh"); return; }
	rddlg = dlg;
    }

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    uiIOObjSelDlg dlg( parnt, *ctio, tr("Select SEG-Y setup") );
    dlg.setModal( true );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    if ( rddlg )
	rddlg->use( ioobj, false );
    else
    {
	FileReadOpts::shallowClear( pars_ );
	pars_.merge( ioobj->pars() );
	defdlg_->use( ioobj, false );
    }
}


class uiSEGYReadPreScanner : public uiDialog
{ mODTextTranslationClass(uiSEGYReadPreScanner);
public:

uiSEGYReadPreScanner( uiParent* p, Seis::GeomType gt, const IOPar& pars )
    : uiDialog(p,uiDialog::Setup(tr("SEG-Y Scan"),mNoDlgTitle,
                                 mODHelpKey(mSEGYReadPreScannerHelpID) ))
    , pars_(pars)
    , geom_(gt)
    , scanner_(0)
    , res_(false)
    , rep_("SEG-Y scan report")
{
    nrtrcsfld_ = new uiGenInput( this, tr("Limit to number of traces"),
				 IntInpSpec(1000) );
    nrtrcsfld_->setWithCheck( true );
    nrtrcsfld_->setChecked( true );

    SEGY::FileSpec fs; fs.usePar( pars );
    BufferString fnm( fs.fileName() );
    fnm.replace( '*', 'x' );
    FilePath fp( fnm ); fp.setExtension( "txt" );
    uiFileInput::Setup fisu( GetProcFileName(fp.fileName()) );
    fisu.forread( false ).objtype( tr("Report") );
    saveasfld_ = new uiFileInput( this, tr("Save report as"), fisu );
    saveasfld_->setWithCheck( true );
    saveasfld_->attach( alignedBelow, nrtrcsfld_ );
    saveasfld_->setChecked( true );

    setModal( true );
}

~uiSEGYReadPreScanner()
{
    delete scanner_;
}

bool acceptOK( CallBacker* )
{
    scanner_= new SEGY::Scanner( pars_, geom_ );
    const int nrtrcs = nrtrcsfld_->isChecked() ? nrtrcsfld_->getIntValue() : 0;
    scanner_->setRichInfo( true );
    scanner_->setMaxNrtraces( nrtrcs );
    uiTaskRunner uitr( this );
    TaskRunner::execute( &uitr, *scanner_ );
    res_ = true;
    if ( scanner_->fileDataSet().isEmpty() )
    {
	uiMSG().error( tr("No traces found") );
	return false;
    }

    const char* fnm = saveasfld_->isChecked() ? saveasfld_->fileName() : 0;
    IOPar rep( "SEG-Y scan report" ); scanner_->getReport( rep );
    uiSEGY::displayReport( parent(), rep, fnm );
    return true;
}

    const Seis::GeomType geom_;
    const IOPar&	pars_;

    uiGenInput*		nrtrcsfld_;
    uiFileInput*	saveasfld_;

    bool		res_;
    IOPar		rep_;
    SEGY::Scanner*	scanner_;

};


void uiSEGYRead::preScanReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    if ( !rddlg ) { pErrMsg("Huh"); return; }
    rddlg->updatePars();
    fillPar( pars_ );

    uiSEGYReadPreScanner dlg( rddlg, geom_, pars_ );
    if ( !dlg.go() || !dlg.res_ ) return;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread, Seis::GeomType gt )
{
    CtxtIOObj* ret = mMkCtxtIOObj( SeisTrc );
    IOObjContext& ctxt = ret->ctxt_;
    ctxt.forread_ = forread;
    ctxt.toselect_.allownonuserselectable_ = true;
    ctxt.fixTranslator( "SEG-Y" );
    IOPar* cnstr = Seis::is2D(gt) ? &ctxt.toselect_.require_
				  : &ctxt.toselect_.dontallow_;
    cnstr->setYN( SeisTrcTranslator::sKeyIs2D(), true );
    cnstr = Seis::isPS(gt) ? &ctxt.toselect_.require_
			   : &ctxt.toselect_.dontallow_;
    cnstr->setYN( SeisTrcTranslator::sKeyIsPS(), true );
    return ret;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread ) const
{
    return getCtio( forread, geom_ );
}


static const uiString rev1info()
{
    return od_static_tr("rev1info","The file was marked as SEG-Y Revision 1 by "
        "the producer.\nUnfortunately, not all files are correct in this "
        "respect.\n\nPlease specify:");
}
static const char* rev1txts[] =
{
    "No: The file is NOT SEG-Y Rev.1 - treat as legacy (i.e. Rev. 0)",
    "Mostly: It's Rev. 1 but I may need to overrule some things",
    "Yes: I know the file is 100% correct SEG-Y Rev.1",
    0
};

class uiSEGYReadRev1Question : public uiVarWizardDlg
{ mODTextTranslationClass(uiSEGYReadRev1Question);
public:

uiSEGYReadRev1Question( uiParent* p, int pol, bool is2d, IOPar& iop )
    : uiVarWizardDlg(p,Setup(tr("Determine SEG-Y revision"),rev1info(),
                             mODHelpKey(mSEGYReadRev1QuestionHelpID) ),
				iop,Middle)
    , initialpol_(pol)
{
    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItems( BufferStringSet(rev1txts) )
		.setChecked( pol-1, true );

    dontaskfld_ = new uiCheckBox( this, tr("Don't ask again for this survey") );
    dontaskfld_->attach( ensureBelow, choicefld_ );
    dontaskfld_->attach( rightBorder );
}

bool acceptOK( CallBacker* )
{
    pol_ = choicefld_->firstChecked() + 1;
    int storepol = dontaskfld_->isChecked() ? -pol_ : pol_;
    if ( storepol != initialpol_ )
    {
	SI().getPars().set( sKeySEGYRev1Pol, storepol );
	SI().savePars();
    }
    pol_--;
    return true;
}

    uiCheckList*	choicefld_;
    uiCheckBox*		dontaskfld_;
    int			pol_, initialpol_;

};

#define mSetreadReqCB() readParsReq.notify( mCB(this,uiSEGYRead,readReq) )
#define mSetwriteReqCB() writeParsReq.notify( mCB(this,uiSEGYRead,writeReq) )
#define mSetpreScanReqCB() preScanReq.notify( mCB(this,uiSEGYRead,preScanReq) )
#define mLaunchDlg(dlg,fn) mLaunchVWDialog(dlg,uiSEGYRead,fn)

void uiSEGYRead::getBasicOpts()
{
    uiSEGYDefDlg::Setup bsu; bsu.geoms_ = setup_.geoms_;
    bsu.defgeom( geom_ ).modal( false );
    defdlg_ = new uiSEGYDefDlg( parent_, bsu, pars_ );
    defdlg_->mSetreadReqCB();
    defdlg_->mSetwriteReqCB();
    mLaunchDlg(defdlg_,defDlgClose);
}


void uiSEGYRead::basicOptsGot()
{
    mHandleVWCancel(defdlg_,cCancelled());
    geom_ = defdlg_->geomType();

    uiSEGYExamine::Setup exsu( geom_, defdlg_->nrTrcExamine() );
    exsu.usePar( pars_ );
    BufferString emsg;
    const int exrev = uiSEGYExamine::getRev( exsu, emsg );
    if ( exrev < 0 )
    {
	rev_ = Rev0;
	if ( emsg.isEmpty() )
	    emsg.set( "Error trying to read traces from file." );
	uiMSG().error( mToUiStringTodo(emsg) );
	// Can't figure out (quickly) how to go on. Wizard will stop
	// better than previously, when it crashed ...
	return;
    }

    delete examdlg_; examdlg_ = 0;
    if ( exsu.nrtrcs_ > 0 )
    {
	examdlg_ = new uiSEGYExamine( parent_, exsu );
        examdlg_->objectToBeDeleted().notify(mCB(this,uiSEGYRead,examDlgClose));
        mLaunchVWDialogOnly( examdlg_, uiSEGYRead, examDlgClose );
    }

    rev_ = exrev ? WeakRev1 : Rev0;
    revpolnr_ = exrev;
    bool needimmediatedet = true;
    if ( rev_ != Rev0 )
    {
	if ( !SI().pars().get(sKeySEGYRev1Pol,revpolnr_) )
	    revpolnr_ = 2;
	if ( revpolnr_ < 0 )
	    revpolnr_ = -revpolnr_ - 1;
	else
	{
	    rev1qdlg_ = new uiSEGYReadRev1Question( parent_, revpolnr_,
						    Seis::is2D(geom_), pars_ );
	    needimmediatedet = false;
	    mLaunchDlg(rev1qdlg_,rev1qDlgClose);
	}
    }

    if ( needimmediatedet )
	determineRevPol();
}



void uiSEGYRead::determineRevPol()
{
    if ( rev1qdlg_ )
    {
	if ( !rev1qdlg_->uiResult() )
	    mSetState(BasicOpts)
	revpolnr_ = rev1qdlg_->pol_;
    }
    rev_ = (RevType)revpolnr_;
    mSetState( setup_.purpose_ == Import ? SetupImport : SetupScan );
}



void uiSEGYRead::setupScan()
{
    delete scanner_; scanner_ = 0;
    uiSEGYReadDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    if ( setup_.purpose_ == SurvSetup && Seis::is2D(geom_) )
	uiMSG().warning(
	tr("Scanning a 2D file can provide valuable info on your survey.\n"
	   "But to actually set up your survey, you need to use\n"
	   "'Set for 2D only'\nIn the survey setup window.\n") );
    scandlg_ = new uiSEGYScanDlg( parent_, su, pars_,
				  setup_.purpose_ == SurvSetup );
    scandlg_->mSetreadReqCB();
    scandlg_->mSetwriteReqCB();
    scandlg_->mSetpreScanReqCB();
    mLaunchDlg(scandlg_,scanDlgClose);
}


void uiSEGYRead::setupImport()
{
    if ( impdlg_ )
        return;

    uiSEGYImpDlg::Setup su( geom_ );
    su.rev( rev_ ).modal(false).wintitle(tr("Import SEG-Y"));
    impdlg_ = new uiSEGYImpDlg( parent_, su, pars_ );
    impdlg_->mSetreadReqCB();
    impdlg_->mSetwriteReqCB();
    impdlg_->mSetpreScanReqCB();
    mLaunchDlg(impdlg_,impDlgClose);
}


void uiSEGYRead::defDlgClose( CallBacker* )
{
    basicOptsGot();
    defdlg_ = 0;
}


void uiSEGYRead::examDlgClose( CallBacker* )
{ examdlg_ = 0; }


void uiSEGYRead::scanDlgClose( CallBacker* )
{
    mHandleVWCancel(scandlg_,BasicOpts)
    scanner_ = scandlg_->getScanner();
    outid_ = scandlg_->outputID();
    scandlg_ = 0;
    mSetState( cFinished() );
}


void uiSEGYRead::impDlgClose( CallBacker* )
{
    mHandleVWCancel(impdlg_,BasicOpts)
    outid_ = impdlg_->outputID();
    impdlg_ = 0;
    mSetState( cFinished() );
}


void uiSEGYRead::rev1qDlgClose( CallBacker* )
{
    mHandleVWCancel(rev1qdlg_,BasicOpts)
    determineRevPol();
    rev1qdlg_ = 0;
}


void uiSEGYRead::raiseCurrent()
{
#define mRaise(dlg) if ( dlg ) dlg->raise()

	 mRaise( defdlg_ );
    else mRaise( impdlg_ );
    else mRaise( scandlg_ );
    else mRaise( examdlg_ );
    else mRaise( rev1qdlg_ );
}
