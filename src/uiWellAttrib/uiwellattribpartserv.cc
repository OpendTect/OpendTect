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
#include "uiwellattribxplot.h"
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
    , crlogcubedlg_(0)
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
    delete welltiedlg_; welltiedlg_ = 0;
    delete crlogcubedlg_; crlogcubedlg_ = 0;

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


bool uiWellAttribPartServer::createAttribLog( const MultiID& wellid )
{
    Well::Data* wd = Well::MGR().get( wellid );
    if ( !wd )
    {
	uiMSG().error( uiStrings::phrCannotRead( uiStrings::sWell() ));
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
    if ( crlogcubedlg_ && wellid!=crlogcubedlg_->currentKey() )
	deleteAndZeroPtr(crlogcubedlg_);

    if ( !crlogcubedlg_ )
    {
	crlogcubedlg_ = new uiCreateLogCubeDlg( parent(), &wellid );
	crlogcubedlg_->setModal( false );
    }

    crlogcubedlg_->show();
    crlogcubedlg_->raise();
    return true;
}


bool uiWellAttribPartServer::createD2TModel( const MultiID& wid )
{
    WellTie::Setup* wtsetup = new WellTie::Setup();
    wtsetup->wellid_ = wid;
    if( welltiedlg_ && (welltiedlg_->getWellId()!=wid) )
    {
	delete welltiedlg_;
	welltiedlg_ = 0;
    }

    if ( !welltiedlg_ )
	welltiedlg_ = new WellTie::uiTieWinMGRDlg( parent(), *wtsetup );
    welltiedlg_->raise();
    welltiedlg_->show();
    return true;
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

    uiAmplSpectrum::Setup su( toUiString(lognm), false,  resamprg.step );
    uiAmplSpectrum* asd = new uiAmplSpectrum( parent(), su );
    asd->setData( resamplvals.arr(), resampsz );
    asd->show();

    return true;
}
