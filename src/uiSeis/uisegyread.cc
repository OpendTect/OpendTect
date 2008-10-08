/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyread.cc,v 1.9 2008-10-08 15:57:32 cvsbert Exp $
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

static const char* sKeySEGYRev1Pol = "SEG-Y Rev. 1 policy";


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

static const int cCancel = -1;
static const int cFinished = 0;
static const int cGetBasicOpts = 1;
static const int cImport = 2;
static const int cSetupScan = 3;
static const int cScanFiles = 4;


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su )
    : setup_(su)
    , parent_(p)
    , geom_(Seis::Vol)
    , state_(cGetBasicOpts)
    , specincomplete_(false)
    , scanner_(0)
    , rev_(Rev0)
    , nrexamine_(0)
{
}


uiSEGYRead::~uiSEGYRead()
{
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
    iop.setYN( SEGY::FileDef::sKeyForceRev0, rev_ == Rev0 );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    if ( iop.isTrue( SEGY::FileDef::sKeyForceRev0 ) ) rev_ = Rev0;
}


void uiSEGYRead::writeReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYImpDlg*,impdlg,cb)
    if ( !impdlg ) return;

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    ctio->ctxt.setName( impdlg->saveObjName() );
    if ( !ctio->fillObj(false) )
	return;

    impdlg->updatePars();
    ctio->ioobj->pars() = pars_;
    SEGY::FileSpec::ensureWellDefined( *ctio->ioobj );
    IOM().commitChanges( *ctio->ioobj );
    delete ctio->ioobj;
}


void uiSEGYRead::readReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYDefDlg*,defdlg,cb)
    mDynamicCastGet(uiSEGYImpDlg*,impdlg,cb)
    uiDialog* parnt = defdlg;
    if ( parnt )
	geom_ = defdlg->geomType();
    else if ( !impdlg )
	return;
    else
	parnt = impdlg;

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    uiIOObjSelDlg dlg( parnt, *ctio, "Select SEG-Y setup" );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    SEGY::FileSpec::fillParFromIOObj( *ioobj, pars_ );
    if ( defdlg )
	defdlg->use( ioobj, false );
    else
	impdlg->use( ioobj, false );
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
    mDynamicCastGet(uiSEGYImpDlg*,impdlg,cb)
    if ( !impdlg ) return;
    impdlg->updatePars();

    uiSEGYReadPreScanner dlg( impdlg, geom_, pars_ );
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


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiSEGYRead::go()
{
    while ( state_ > 0 )
    {
	if ( state_ == cGetBasicOpts )
	    getBasicOpts();
	else if ( state_ == cImport )
	    doImport();
	else if ( state_ == cSetupScan )
	    setupScan();
	else if ( state_ == cScanFiles )
	    doScan();
    }

    return state_ == 0;
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
    : uiDialog(p,Setup("Determine SEG-Y revision",rev1info,mNoHelpID) )
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

void uiSEGYRead::getBasicOpts()
{
    uiSEGYDefDlg::Setup bsu; bsu.geoms_ = setup_.geoms_;
    bsu.defgeom( geom_ );
    uiSEGYDefDlg dlg( parent_, bsu, pars_ );
    dlg.mSetreadReqCB();
    if ( !dlg.go() )
	{ state_ = cCancel; return; }
    geom_ = dlg.geomType();
    nrexamine_ =  dlg.nrTrcExamine();

    uiSEGYExamine::Setup exsu( nrexamine_ ); exsu.usePar( pars_ );
    int exrev = uiSEGYExamine::getRev( exsu );
    if ( exrev < 0 )
	{ rev_ = Rev0; state_ = cGetBasicOpts; return; }

    rev_ = exrev ? WeakRev1 : Rev0;
    if ( rev_ != Rev0 )
    {
	int pol = 2; SI().pars().get( sKeySEGYRev1Pol, pol );
	if ( pol < 0 )
	    pol = -pol;
	else
	{
	    uiSEGYReadRev1Question dlg( parent_, pol );
	    if ( !dlg.go() || dlg.isGoBack() )
		{ state_ = cGetBasicOpts; return; }
	    pol = dlg.pol_;
	}

	rev_ = pol == 1 ? Rev1 : (pol == 2 ? WeakRev1 : Rev0);
    }

    state_ = setup_.purpose_ == Import ? cImport
	   : (rev_ != Rev1 ? cSetupScan : cScanFiles);
}


void uiSEGYRead::setupScan()
{
}


void uiSEGYRead::doScan()
{
    delete scanner_;

    SEGY::FileSpec fs; fs.usePar( pars_ );
    scanner_ = new SEGY::Scanner( fs, geom_, pars_ );
    uiTaskRunner tr( parent_ );
    if ( !tr.execute(*scanner_) )
	state_ = specincomplete_ ? cSetupScan : cGetBasicOpts;

    state_ = cFinished;
}


void uiSEGYRead::doImport()
{
    uiSEGYImpDlg::Setup su( geom_ );
    su.rev( rev_ ).nrexamine( nrexamine_ );
    uiSEGYImpDlg dlg( parent_, su, pars_ );
    dlg.mSetreadReqCB();
    dlg.mSetwriteReqCB();
    dlg.mSetpreScanReqCB();
    state_ = dlg.go() ? cFinished : cGetBasicOpts;
}
