/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyread.cc,v 1.12 2008-10-15 15:47:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyread.h"
#include "uisegydef.h"
#include "uisegydefdlg.h"
#include "uisegyimpdlg.h"
#include "uisegyexamine.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifileinput.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uiobjdisposer.h"
#include "uimsg.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seisscanner.h"
#include "segyscanner.h"
#include "seisioobjinfo.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "filepath.h"
#include "timefun.h"
#include <sstream>

static const char* sKeySEGYRev1Pol = "SEG-Y Rev. 1 policy";

#define mSetState(st) { state_ = st; nextAction(); return; }


void uiSEGYRead::Setup::getDefaultTypes( TypeSet<Seis::GeomType>& geoms )
{
    if ( SI().has3D() )
    {
	geoms += Seis::Vol;
	geoms += Seis::VolPS;
    }
    if ( SI().has2D() )
    {
	geoms += Seis::Line;
	geoms += Seis::LinePS;
    }
}


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su )
    : setup_(su)
    , parent_(p)
    , geom_(SI().has3D()?Seis::Vol:Seis::Line)
    , state_(BasicOpts)
    , scanner_(0)
    , rev_(Rev0)
    , revpolnr_(2)
    , defdlg_(0)
    , examdlg_(0)
    , impdlg_(0)
    , rev1qdlg_(0)
    , processEnded(this)
{
    nextAction();
}

// Destructor at end of file (deleting local class)


void uiSEGYRead::closeDown()
{
    processEnded.trigger();
    uiOBJDISP()->go( this );
}


void uiSEGYRead::nextAction()
{
    if ( state_ <= Finished )
	{ closeDown(); return; }

    switch ( state_ )
    {
    case Wait4Dialog:
	return;
    break;
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
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIs2D, is2d );
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIsPS, isps );
    geom_ = Seis::geomTypeOf( is2d, isps );
}


void uiSEGYRead::use( const IOObj* ioobj, bool force )
{
    if ( !ioobj ) return;

    pars_.merge( ioobj->pars() );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
	setGeomType( *ioobj );
}


void uiSEGYRead::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    if ( rev_ == Rev0 )
	iop.setYN( SEGY::FileDef::sKeyForceRev0, true );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    if ( iop.isTrue( SEGY::FileDef::sKeyForceRev0 ) ) rev_ = Rev0;
}


void uiSEGYRead::writeReq( CallBacker* cb )
{
    if ( cb != impdlg_ ) return;

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    ctio->ctxt.setName( impdlg_->saveObjName() );
    if ( !ctio->fillObj(false) )
	return;

    impdlg_->updatePars();
    fillPar( ctio->ioobj->pars() );
    ctio->ioobj->pars().removeWithKey( uiSEGYExamine::Setup::sKeyNrTrcs );
    ctio->ioobj->pars().removeWithKey( sKey::Geometry );
    SEGY::FileSpec::ensureWellDefined( *ctio->ioobj );
    IOM().commitChanges( *ctio->ioobj );
    delete ctio->ioobj;
}


void uiSEGYRead::readReq( CallBacker* cb )
{
    uiDialog* parnt = defdlg_;
    if ( parnt )
	geom_ = defdlg_->geomType();
    else if ( !impdlg_ )
	return;
    else
	parnt = impdlg_;

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    uiIOObjSelDlg dlg( parnt, *ctio, "Select SEG-Y setup" );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    if ( impdlg_ )
	impdlg_->use( ioobj, false );
    else
    {
	pars_.merge( ioobj->pars() );
	defdlg_->use( ioobj, false );
    }
}


class uiSEGYReadPreScanner : public uiDialog
{
public:

uiSEGYReadPreScanner( uiParent* p, Seis::GeomType gt, const IOPar& pars )
    : uiDialog(p,uiDialog::Setup("SEG-Y Scan",0,mNoHelpID))
    , pars_(pars)
    , geom_(gt)
    , scanner_(0)
    , res_(false)
    , rep_("SEG-Y scan report")
{
    nrtrcsfld_ = new uiGenInput( this, "Limit to number of traces",
	    			 IntInpSpec(1000) );
    nrtrcsfld_->setWithCheck( true );
    nrtrcsfld_->setChecked( true );

    SEGY::FileSpec fs; fs.usePar( pars );
    BufferString fnm( fs.fname_ );
    replaceCharacter( fnm.buf(), '*', 'x' );
    FilePath fp( fnm );
    fp.setExtension( "txt" );
    saveasfld_ = new uiFileInput( this, "Save report as",
	    			  GetProcFileName(fp.fileName()) );
    saveasfld_->setWithCheck( true );
    saveasfld_->attach( alignedBelow, nrtrcsfld_ );
    saveasfld_->setChecked( true );
}

~uiSEGYReadPreScanner()
{
    delete scanner_;
}

bool acceptOK( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->isChecked() ? nrtrcsfld_->getIntValue() : 0;
    const char* fnm = saveasfld_->isChecked() ? saveasfld_->fileName() : "";

    scanner_= new SEGY::Scanner( pars_, geom_ );
    scanner_->setMaxNrtraces( nrtrcs );
    uiTaskRunner uitr( this );
    uitr.execute( *scanner_ );
    res_ = true;
    if ( scanner_->fileData().isEmpty() )
    {
	uiMSG().error( "No traces found" );
	res_ = false;
    }
    else
    {
	scanner_->getReport( rep_ );
	if ( *fnm && ! rep_.write(fnm,IOPar::sKeyDumpPretty) )
	    uiMSG().warning( "Cannot write report to specified file" );
    }

    uiDialog* dlg = new uiDialog( parent(), uiDialog::Setup("SEG-Y Scan Report",
				  mNoDlgTitle,mNoHelpID).modal(false) );
    dlg->setCtrlStyle( uiDialog::LeaveOnly );
    std::ostringstream strstrm; rep_.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, "SEG-Y Scan Report" );
    te->setText( strstrm.str().c_str() );
    dlg->setDeleteOnClose( true ); dlg->go();
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
    if ( !impdlg_ ) return;
    impdlg_->updatePars();
    fillPar( pars_ );

    uiSEGYReadPreScanner dlg( impdlg_, geom_, pars_ );
    if ( !dlg.go() || !dlg.res_ ) return;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread ) const
{
    CtxtIOObj* ret = mMkCtxtIOObj( SeisTrc );
    IOObjContext& ctxt = ret->ctxt;
    ctxt.trglobexpr = "SEG-Y";
    ctxt.forread = forread;
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIs2D, Seis::is2D(geom_) );
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIsPS, Seis::isPS(geom_) );
    ctxt.includeconstraints = ctxt.allownonreaddefault = true;
    ctxt.allowcnstrsabsent = false;
    return ret;
}


static const char* rev1info =
    "The file was marked as SEG-Y Revision 1 by the producer."
    "\nUnfortunately, not all files are correct in this respect."
    "\n\nPlease specify:";
static const char* rev1txts[] =
{
    "Yes - I know the file is 100% correct SEG-Y Rev.1",
    "Yes - It's Rev. 1 but I may need to overrule some things",
    "No - the file is not SEG-Y Rev.1 - treat as legacy SEG-Y Rev. 0",
    "Cancel - Something must be wrong - take me back",
    0
};

class uiSEGYReadRev1Question : public uiDialog
{
public:

uiSEGYReadRev1Question( uiParent* p, int pol )
    : uiDialog(p,Setup("Determine SEG-Y revision",rev1info,mNoHelpID)
	    	.modal(false) )
    , initialpol_(pol)
{
    uiButtonGroup* bgrp = new uiButtonGroup( this, "" );
    for ( int idx=0; rev1txts[idx]; idx++ )
	buts_ += new uiRadioButton( bgrp, rev1txts[idx] );
    bgrp->setRadioButtonExclusive( true );
    buts_[pol-1]->setChecked( true );

    dontaskfld_ = new uiCheckBox( this, "Don't ask again for this survey" );
    dontaskfld_->attach( alignedBelow, bgrp );
}

bool acceptOK( CallBacker* )
{
    pol_ = 3;
    for ( int idx=0; idx<buts_.size(); idx++ )
    {
	if ( buts_[idx]->isChecked() )
	    { pol_ = idx + 1; break; }
    }
    int storepol = dontaskfld_->isChecked() ? -pol_ : pol_;
    if ( storepol != initialpol_ )
    {
	SI().getPars().set( sKeySEGYRev1Pol, storepol );
	SI().savePars();
    }
    return true;
}

bool isGoBack() const
{
    return pol_ == buts_.size();
}

    ObjectSet<uiRadioButton>	buts_;
    uiCheckBox*			dontaskfld_;
    int				pol_, initialpol_;

};

#define mSetreadReqCB() readParsReq.notify( mCB(this,uiSEGYRead,readReq) )
#define mSetwriteReqCB() writeParsReq.notify( mCB(this,uiSEGYRead,writeReq) )
#define mSetpreScanReqCB() preScanReq.notify( mCB(this,uiSEGYRead,preScanReq) )
#define mLaunchDlg(dlg,fn) \
	dlg->windowClosed.notify( mCB(this,uiSEGYRead,fn) ); \
	dlg->setDeleteOnClose( true ); dlg->go()

void uiSEGYRead::getBasicOpts()
{
    delete defdlg_;
    uiSEGYDefDlg::Setup bsu; bsu.geoms_ = setup_.geoms_;
    bsu.defgeom( geom_ ).modal( false );
    defdlg_ = new uiSEGYDefDlg( parent_, bsu, pars_ );
    defdlg_->mSetreadReqCB();
    mLaunchDlg(defdlg_,defDlgClose);
    mSetState(Wait4Dialog);
}


void uiSEGYRead::basicOptsGot()
{
    if ( !defdlg_->uiResult() )
	{ delete defdlg_; defdlg_ = 0; mSetState(Cancelled); }
    geom_ = defdlg_->geomType();

    uiSEGYExamine::Setup exsu( defdlg_->nrTrcExamine() );
    exsu.modal( false ); exsu.usePar( pars_ );
    delete examdlg_; examdlg_ = new uiSEGYExamine( parent_, exsu );
    mLaunchDlg(examdlg_,examDlgClose);
    const int exrev = examdlg_->getRev();
    if ( exrev < 0 )
	{ rev_ = Rev0; mSetState(BasicOpts); }

    rev_ = exrev ? WeakRev1 : Rev0;
    if ( rev_ != Rev0 )
    {
	SI().pars().get( sKeySEGYRev1Pol, revpolnr_ );
	if ( revpolnr_ < 0 )
	    revpolnr_ = -revpolnr_;
	else
	{
	    delete rev1qdlg_;
	    rev1qdlg_ = new uiSEGYReadRev1Question( parent_, revpolnr_ );
	    mLaunchDlg(rev1qdlg_,rev1qDlgClose);
	    mSetState(Wait4Dialog);
	}
    }

    determineRevPol();
}



void uiSEGYRead::determineRevPol()
{
    if ( rev1qdlg_ )
    {
	if ( !rev1qdlg_->uiResult() || rev1qdlg_->isGoBack() )
	    mSetState(BasicOpts)
	revpolnr_ = rev1qdlg_->pol_;
    }
    rev_ = revpolnr_ == 1 ? Rev1 : (revpolnr_ == 2 ? WeakRev1 : Rev0);
    mSetState( setup_.purpose_ == Import ? SetupImport : SetupScan );
}



void uiSEGYRead::setupScan()
{
    //TODO get ReadOpts and scan pars

    SEGY::FileSpec fs; fs.usePar( pars_ );
    delete scanner_; scanner_ = new SEGY::Scanner( fs, geom_, pars_ );
    if ( rev_ == Rev0 )
	scanner_->setForceRev0( true );
    uiTaskRunner tr( parent_ );
    if ( !tr.execute(*scanner_) )
	mSetState( SetupScan )

    mSetState( Finished );
}


void uiSEGYRead::setupImport()
{
    delete impdlg_;
    uiSEGYImpDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    impdlg_ = new uiSEGYImpDlg( parent_, su, pars_ );
    impdlg_->mSetreadReqCB();
    impdlg_->mSetwriteReqCB();
    impdlg_->mSetpreScanReqCB();
    mLaunchDlg(impdlg_,impDlgClose);
    mSetState( Wait4Dialog );
}


void uiSEGYRead::defDlgClose( CallBacker* )
{
    basicOptsGot();
    defdlg_ = 0;
}


void uiSEGYRead::examDlgClose( CallBacker* )
{
    examdlg_ = 0;
}


void uiSEGYRead::impDlgClose( CallBacker* )
{
    State newstate = impdlg_->uiResult() ? Finished : BasicOpts;
    impdlg_ = 0;
    mSetState( newstate );
}


void uiSEGYRead::rev1qDlgClose( CallBacker* )
{
    determineRevPol();
    rev1qdlg_ = 0;
}


uiSEGYRead::~uiSEGYRead()
{
    delete defdlg_;
    delete examdlg_;
    delete impdlg_;
    delete rev1qdlg_;
}
