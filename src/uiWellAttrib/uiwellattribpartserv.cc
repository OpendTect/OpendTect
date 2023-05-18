/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribpartserv.h"

#include "uiamplspectrum.h"
#include "uibuttongroup.h"
#include "uicreateattriblogdlg.h"
#include "uicreatelogcubedlg.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiwellattribxplot.h"
#include "uiwellman.h"
#include "uiwelltiemgrdlg.h"
#include "uiwellto2dlinedlg.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "ioobj.h"
#include "ioman.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltiesetup.h"

using namespace Attrib;


int uiWellAttribPartServer::evPreview2DFromWells()	{ return 0; }
int uiWellAttribPartServer::evShow2DFromWells()		{ return 1; }
int uiWellAttribPartServer::evCleanPreview()		{ return 2; }

uiWellAttribPartServer::uiWellAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , attrset_(new DescSet(false)) //Default, set afterwards
{
    mAttachCB( IOM().surveyChanged, uiWellAttribPartServer::surveyChangedCB );
    mAttachCB( uiWellMan::instanceCreated(),
	       uiWellAttribPartServer::wellManCreatedCB );
}


uiWellAttribPartServer::~uiWellAttribPartServer()
{
    detachAllNotifiers();
    cleanUp();
}


void uiWellAttribPartServer::surveyChangedCB( CallBacker* )
{
    cleanUp();
}


void uiWellAttribPartServer::wellManCreatedCB( CallBacker* cb )
{
    mDynamicCastGet(uiWellMan*,wm,cb)
    if ( !wm ) return;

    new uiToolButton( wm->extraButtonGroup(), "xplot_wells",
		      uiStrings::sCrossPlot(),
		      mCB(this,uiWellAttribPartServer,xplotCB) );
}


static const Attrib::DescSet* getUserPrefDescSet()
{
    const DescSet* ds3d = DSHolder().getDescSet( false, false );
    const DescSet* ds2d = DSHolder().getDescSet( true, false );
    if ( !ds3d && !ds2d ) return nullptr;
    if ( !(ds3d && ds2d) ) return ds3d ? ds3d : ds2d;
    if ( !SI().has3D() ) return ds2d;
    if ( !SI().has2D() ) return ds3d;

    const int nr3d = ds3d->nrDescs( false, true );
    const int nr2d = ds2d->nrDescs( false, true );
    if ( (nr3d>0) != (nr2d>0) ) return nr2d > 0 ? ds2d : ds3d;

    const int res =
	uiMSG().ask2D3D( toUiString("Which attributes do you want to use?"),
	true );
    return res==-1 ? nullptr : (res==1 ? ds2d : ds3d);
}


void uiWellAttribPartServer::xplotCB( CallBacker* )
{
    const DescSet* ds = getUserPrefDescSet();
    if ( !ds )
	return;

    setAttribSet( *ds );
    doXPlot();
}


void uiWellAttribPartServer::cleanUp()
{
    deleteAndNullPtr( attrset_ );
    closeAndNullPtr( xplotwin2d_ );
    closeAndNullPtr( xplotwin3d_ );
    closeAndNullPtr( welltiedlg_ );
    closeAndNullPtr( crlogcubedlg_ );
    closeAndNullPtr( wellto2ddlg_ );
}


void uiWellAttribPartServer::setAttribSet( const DescSet& ads )
{
    delete attrset_;
    attrset_ = new DescSet( ads );
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
	mAttachCB( wellto2ddlg_->wantspreview_,
		   uiWellAttribPartServer::previewWellto2DLine );
	mAttachCB( wellto2ddlg_->windowClosed,
		   uiWellAttribPartServer::wellTo2DDlgClosed );
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
    RefMan<Well::Data> wd = Well::MGR().get(wellid, Well::LoadReqs(Well::Inf));
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
	closeAndNullPtr( crlogcubedlg_ );

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
    WellTie::Setup wtsetup;
    wtsetup.wellid_ = wid;
    if( welltiedlg_ && welltiedlg_->getWellId() != wid )
	closeAndNullPtr( welltiedlg_ );

    if ( !welltiedlg_ )
    {
	welltiedlg_ = new WellTie::uiTieWinMGRDlg( parent(), wtsetup );
	welltiedlg_->setDeleteOnClose( true );
	mAttachCB( welltiedlg_->windowClosed,
		   uiWellAttribPartServer::welltieDlgClosedCB );
    }

    welltiedlg_->raise();
    welltiedlg_->show();
    return true;
}


void uiWellAttribPartServer::welltieDlgClosedCB( CallBacker* )
{
    welltiedlg_ = nullptr;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWellAttribPartServer::showAmplSpectrum( const MultiID& mid,
					       const char* lognm )
{
    ConstRefMan<Well::Data> wd = Well::MGR().get( mid,
					       Well::LoadReqs(Well::LogInfos) );
    if ( !wd || wd->logs().isEmpty()  )
	return false;

    const Well::Log* log = wd->getLog( lognm );
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
    auto* asd = new uiAmplSpectrum( parent(), su );
    asd->setData( resamplvals.arr(), resampsz );
    asd->show();

    return true;
}
