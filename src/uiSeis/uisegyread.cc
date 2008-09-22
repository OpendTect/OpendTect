/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyread.cc,v 1.1 2008-09-22 15:09:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyread.h"
#include "uisegydef.h"
#include "uisegydefdlg.h"
#include "uisegyexamine.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "survinfo.h"
#include "segytr.h"
#include "segyscanner.h"
#include "seisioobjinfo.h"


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
static const int cGetFileOpts = 2;
static const int cScanFiles = 3;
static const int cImport = 4;


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su )
    : setup_(su)
    , parent_(p)
    , geom_(Seis::Vol)
    , state_(cGetBasicOpts)
    , needsetupdlg_(false)
    , scanner_(0)
{
}


uiSEGYRead::~uiSEGYRead()
{
}


void uiSEGYRead::use( const IOObj* ioobj, bool force )
{
    if ( !ioobj ) return;

    pars_.merge( ioobj->pars() );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
	geom_ = oinf.geomType();
}


void uiSEGYRead::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    iop.setYN( SEGYSeisTrcTranslator::sForceRev0, rev_ == 0 );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    if ( iop.isTrue( SEGYSeisTrcTranslator::sForceRev0 ) ) rev_ = 0;
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiSEGYRead::go()
{
    while ( state_ > 0 )
    {
	if ( state_ == cGetBasicOpts )
	    getBasicOpts();
	else if ( state_ == cGetFileOpts )
	    getFileOpts();
	else if ( state_ == cScanFiles )
	    doScan();
	else if ( state_ == cImport )
	    doImport();
    }

    return state_ == 0;
}


void uiSEGYRead::launchExaminer( int nrtrcs )
{
    uiSEGYExamine::Setup su( nrtrcs );
    su.usePar( pars_ );
    uiSEGYExamine* exdlg = new uiSEGYExamine( 0, su );
    exdlg->go();
    rev_ = exdlg->getRev();
}


static const char* rev1info =
    "The file was marked as SEG-Y Revision 1 by the producer."
    "\nUnfortunately, not all files are correct in this respect."
    "\n\nPlease specify:";
static const char* rev1txts[] =
{
    "Yes - I know the file is 100% correct SEG-Y Rev.1",
    "Yes - but I may need to overrule some things",
    "No - the file is not SEG-Y Rev.1 - treat as legacy SEG-Y Rev. 0",
    "Cancel - Something's wrong - take me back",
    0
};

void uiSEGYRead::getBasicOpts()
{
    uiSEGYBasic::Setup su; su.geoms_ = setup_.geoms_;
    uiSEGYBasic dlg( parent_, su, pars_ );
    if ( !dlg.go() )
	{ state_ = cCancel; return; }
    geom_ = dlg.geomType();

    launchExaminer( dlg.nrTrcExamine() );
    if ( rev_ < 0 )
	{ state_ = cGetBasicOpts; return; }

    needsetupdlg_ = true;
    if ( rev_ > 0 )
    {
	uiDialog dlg( parent_, uiDialog::Setup("Determine SEG-Y revision",
						rev1info,mNoHelpID) );
	uiButtonGroup* bgrp = new uiButtonGroup( &dlg, "" );
	uiRadioButton* allok = new uiRadioButton( bgrp, rev1txts[0] );
	uiRadioButton* rev1except = new uiRadioButton( bgrp, rev1txts[1] );
	uiRadioButton* notrev1 = new uiRadioButton( bgrp, rev1txts[2] );
	uiRadioButton* goback = new uiRadioButton( bgrp, rev1txts[3] );
	bgrp->setRadioButtonExclusive( true );
	allok->setChecked( true );
	if ( !dlg.go() || goback->isChecked() )
	    { state_ = cGetBasicOpts; return; }
	else if ( allok->isChecked() )
	    needsetupdlg_ = false;
	else if ( notrev1->isChecked() )
	    rev_ = 0;
    }

    state_ = needsetupdlg_ ? cGetFileOpts : targetState();
}


int uiSEGYRead::targetState() const
{
    return setup_.purpose_ == Import ? cImport : cScanFiles;
}


void uiSEGYRead::getFileOpts()
{
    uiSEGYFileOptsDlg::Setup su( geom_, setup_.purpose_ );
    su.isrev1( rev_ != 0 );
    uiSEGYFileOptsDlg dlg( parent_, su, pars_ );
    state_ = dlg.go() ? targetState() : cGetBasicOpts;
}


void uiSEGYRead::doScan()
{
    delete scanner_;

    SEGY::FileSpec fs; fs.usePar( pars_ );
    scanner_ = new SEGY::Scanner( fs, geom_, pars_ );
    uiTaskRunner tr( parent_ );
    if ( !tr.execute(*scanner_) )
	state_ = needsetupdlg_ ? cGetFileOpts : cGetBasicOpts;

    state_ = cFinished;
}


void uiSEGYRead::doImport()
{
    state_ = cCancel;
}
