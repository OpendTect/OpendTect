/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwellattribpartserv.h"

#include "uiamplspectrum.h"
#include "uichecklist.h"
#include "uicreateattriblogdlg.h"
#include "uicreatelogcubedlg.h"
#include "uimsg.h"
#include "uisegyread.h"
#include "uiwellattribxplot.h"
#include "uiwellimpsegyvsp.h"
#include "uiwelltiemgrdlg.h"
#include "uiwellto2dlinedlg.h"

#include "attribdescset.h"
#include "datapointset.h"
#include "ioobj.h"
#include "ioman.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "randcolor.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltiesetup.h"


int uiWellAttribPartServer::evPreview2DFromWells()	{ return 0; }
int uiWellAttribPartServer::evShow2DFromWells()		{ return 1; }
int uiWellAttribPartServer::evCleanPreview()		{ return 2; }

uiWellAttribPartServer::uiWellAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset_(new Attrib::DescSet(false)) //Default, set afterwards
    , nlamodel_(0)
    , xplotwin2d_(0)
    , xplotwin3d_(0)
    , dpsdispmgr_(0)
    , welltiedlg_(0)
    , wellto2ddlg_(0)
    , cursegyread_(0)
    , welltiedlgopened_(false)
{
    IOM().surveyChanged.notify(
	    mCB(this,uiWellAttribPartServer,surveyChangedCB) );
}


uiWellAttribPartServer::~uiWellAttribPartServer()
{
    cleanUp();
}



void uiWellAttribPartServer::surveyChangedCB( CallBacker* )
{
    cleanUp();
}


void uiWellAttribPartServer::cleanUp()
{
    delete attrset_; attrset_ = 0;
    delete xplotwin2d_; xplotwin2d_ = 0;
    delete xplotwin3d_; xplotwin3d_ = 0;

    if ( welltiedlg_ )
    {
	welltiedlg_->windowClosed.remove(
		mCB(this,uiWellAttribPartServer,closeWellTieDlg) );
	welltiedlg_->delWins();
	delete welltiedlg_; welltiedlg_ = 0;
    }

    if ( wellto2ddlg_ )
    {
	wellto2ddlg_->wantspreview_.remove(
		mCB(this,uiWellAttribPartServer,previewWellto2DLine) );
	delete wellto2ddlg_; wellto2ddlg_ = 0;
    }
}


void uiWellAttribPartServer::setAttribSet( const Attrib::DescSet& ads )
{
    delete attrset_;
    attrset_ = new Attrib::DescSet( ads );
}


void uiWellAttribPartServer::setNLAModel( const NLAModel* mdl )
{
    nlamodel_ = mdl;
}


void uiWellAttribPartServer::previewWellto2DLine( CallBacker* )
{
    sendEvent( evPreview2DFromWells() );
}


bool uiWellAttribPartServer::create2DFromWells( MultiID& seisid,
						Pos::GeomID& geomid )
{
    if ( !wellto2ddlg_ )
    {
	wellto2ddlg_  = new uiWellTo2DLineDlg( parent() );
	wellto2ddlg_->wantspreview_.notify(
		mCB(this,uiWellAttribPartServer,previewWellto2DLine) );
	wellto2ddlg_->windowClosed.notify(
		mCB(this,uiWellAttribPartServer,wellTo2DDlgClosed) );
    }

    if ( wellto2ddlg_->go() )
    {
	seisid = wellto2ddlg_->get2DDataSetObj()->key();
	geomid = wellto2ddlg_->get2DLineID();
	if ( wellto2ddlg_->dispOnCreation() )
	    sendEvent( evShow2DFromWells() );
	return true;
    }

    return false;
}


void uiWellAttribPartServer::wellTo2DDlgClosed( CallBacker* )
{
    sendEvent( evCleanPreview() );
}


Pos::GeomID uiWellAttribPartServer::new2DFromWellGeomID() const
{
    return wellto2ddlg_ ? wellto2ddlg_->get2DLineID()
			: Survey::GeometryManager::cUndefGeomID();
}


bool uiWellAttribPartServer::getPrev2DFromWellCoords( TypeSet<Coord>& coords )
{
    if ( !wellto2ddlg_ )
	return false;
    wellto2ddlg_->getCoordinates( coords );
    return true;
}


void uiWellAttribPartServer::doXPlot()
{
    const bool is2d = attrset_->is2D();

    uiWellAttribCrossPlot*& xplotwin = is2d ? xplotwin2d_ : xplotwin3d_;
    if ( !xplotwin )
	xplotwin = new uiWellAttribCrossPlot( parent(), attrset_ );
    else
	xplotwin->setDescSet( attrset_ );

    xplotwin->setDisplayMgr( dpsdispmgr_ );
    xplotwin->show();
}


void uiWellAttribPartServer::doSEGYTool( IOPar* previop, int choice )
{
    if ( choice < 0 )
    {
	uiDialog dlg( parent(), uiDialog::Setup(tr("SEG-Y Tool"),
	    previop ? tr("Import more data?") : tr("What do you want to do?"),
	    mTODOHelpKey) );
	uiCheckList* choicefld = new uiCheckList( &dlg, uiCheckList::OneOnly );
	choicefld->addItem( tr("Import SEG-Y file(s) to OpendTect data") )
	    .addItem( tr("Scan SEG-Y file(s) to use in-place") )
	    .addItem( tr("Import VSP data from SEG-Y file") )
	    .addItem( previop ? tr("Quit SEG-Y import")
			      : tr("Cancel the operation") )
	    .setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice = choicefld->firstChecked();
    }

    switch ( choice )
    {
    case 0: case 1:
	if ( cursegyread_ )
	    { cursegyread_->raiseCurrent(); return; }
	cursegyread_ = 0;
	if ( !launchSEGYWiz(previop,choice) )
	    return;
    break;
    case 2:
	doVSPTool( previop );
	return;
    break;
    default:
    return;
    }
}


void uiWellAttribPartServer::doVSPTool( IOPar* previop, int choice )
{
    if ( choice < 0 )
    {
	uiDialog dlg( parent(), uiDialog::Setup(tr("VSP Import"),
					mNoDlgTitle,mTODOHelpKey) );
	uiCheckList* choicefld = new uiCheckList( &dlg, uiCheckList::OneOnly );
	choicefld->addItem(tr("2-D VSP (will be imported as 2-D seismic line)"))
	    .addItem( tr("3-D VSP (can only be imported as 3D cube)") )
	    .addItem( tr("Zero-offset (single trace) VSP") )
	    .addItem( previop ? tr("Quit import") : tr("Cancel the operation") )
	    .setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice = choicefld->firstChecked();
    }

    switch ( choice )
    {
	case 0: case 1: {
	    const Seis::GeomType gt( choice == 0 ? Seis::Line : Seis::Vol );
	    IOPar iop; if ( previop ) iop = *previop;
	    putInPar( gt, iop );
	    if ( !launchSEGYWiz(&iop,0) )
		return;
	break; }
	case 2: {
	    uiWellImportSEGYVSP dlg( parent() );
	    if ( !dlg.go() )
		return;
	break; }
	default:
	    return;
    }
}


bool uiWellAttribPartServer::launchSEGYWiz( IOPar* previop, int choice )
{
    if ( cursegyread_ )
    {
	uiMSG().error( tr("Please finish your current SEG-Y import first") );
	cursegyread_->raiseCurrent();
	return false;
    }

    switch ( choice )
    {
	case 0:
	    cursegyread_ = new uiSEGYRead( parent(),
				uiSEGYRead::Setup(uiSEGYRead::Import) );
	break;
	case 1: {
	    uiSEGYRead::Setup su( uiSEGYRead::DirectDef );
	    su.geoms_ -= Seis::Line;
	    cursegyread_ = new uiSEGYRead( parent(), su );
	break; }
	default:
	    return false;
    }

    if ( previop )
	cursegyread_->pars().merge( *previop );
    cursegyread_->processEnded.notify(
			mCB(this,uiWellAttribPartServer,segyToolEnded) );
    return true;
}


void uiWellAttribPartServer::segyToolEnded( CallBacker* )
{
    if ( !cursegyread_ || cursegyread_->state() == uiVarWizard::cCancelled() )
    { cursegyread_ = 0; return; }

    IOPar previop( cursegyread_->pars() );
    cursegyread_ = 0;

    doSEGYTool( &previop );
}


bool uiWellAttribPartServer::createAttribLog( const MultiID& wellid )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd )
    {
	uiMSG().error( tr("Cannot read well data") );
	return false;
    }

    BufferStringSet wellnames;
    wellnames.add( wd->name() );
    return createAttribLog( wellnames );
}


bool uiWellAttribPartServer::createAttribLog( const BufferStringSet& wellnames )
{
    uiCreateAttribLogDlg dlg( appserv().parent(), wellnames, attrset_,
			      nlamodel_, wellnames.size() == 1 );
    return dlg.go();
}


bool uiWellAttribPartServer::createLogCube( const MultiID& wellid )
{
    uiCreateLogCubeDlg dlg( parent(), &wellid );
    return dlg.go();
}


bool uiWellAttribPartServer::createD2TModel( const MultiID& wid )
{
    WellTie::Setup* wtsetup = new WellTie::Setup();
    wtsetup->wellid_ = wid;
    if ( !welltiedlgopened_ )
    {
	welltiedlg_ = new WellTie::uiTieWinMGRDlg( parent(), *wtsetup );
	welltiedlg_->windowClosed.notify(
		mCB(this,uiWellAttribPartServer,closeWellTieDlg));
	welltiedlgopened_ = welltiedlg_->go();
    }
    return true;
}


void uiWellAttribPartServer::closeWellTieDlg( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWinMGRDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    dlg->delWins();
    welltiedlgopened_ = false;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWellAttribPartServer::showAmplSpectrum( const MultiID& mid,
					       const char* lognm )
{
    const RefMan<Well::Data> wd = Well::MGR().get( mid );
    if ( !wd || wd->logs().isEmpty()  )
	return false;

    const Well::Log* log = wd->logs().getLog( lognm );
    if ( !log )
	mErrRet( tr("Cannot find log in well data."
		    "  Probably it has been deleted") )

    if ( !log->size() )
	mErrRet( tr("Well log is empty") )

    StepInterval<float> resamprg( log->dahRange() );
    TypeSet<float> resamplvals; int resampsz = 0;
    if ( SI().zIsTime() && wd->haveD2TModel() )
    {
	const Well::D2TModel& d2t = *wd->d2TModel();
	resamprg.set(d2t.getTime(resamprg.start, wd->track()),
		     d2t.getTime(resamprg.stop, wd->track()),1);
	resamprg.step /= SI().zDomain().userFactor();
	resampsz = resamprg.nrSteps();
	for ( int idx=0; idx<resampsz; idx++ )
	{
	    const float dah = d2t.getDah( resamprg.atIndex( idx ),
					  wd->track() );
	    resamplvals += log->getValue( dah );
	}
    }
    else
    {
	resampsz = resamprg.nrSteps();
	resamprg.step = resamprg.width() / (float)log->size();
	for ( int idx=0; idx<resampsz; idx++ )
	    resamplvals += log->getValue( resamprg.atIndex( idx ) );
    }

    uiAmplSpectrum::Setup su( lognm, false,  resamprg.step );
    uiAmplSpectrum* asd = new uiAmplSpectrum( parent(), su );
    asd->setData( resamplvals.arr(), resampsz );
    asd->show();

    return true;
}

