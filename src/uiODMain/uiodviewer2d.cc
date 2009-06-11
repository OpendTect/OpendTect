/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodviewer2d.cc,v 1.3 2009-06-11 11:00:08 cvsnanne Exp $";

#include "uiodviewer2d.h"

#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribdataholder.h"
#include "attribsel.h"
#include "emhorizonpainter.h"
#include "emmanager.h"
#include "horflatvieweditor.h"
#include "settings.h"


uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , selspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , horpainter_(0)
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    appl.applMgr().visServer()->getObjectInfo( visid, info );
    basetxt_ += info;
}


uiODViewer2D::~uiODViewer2D()
{
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin_)
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete horpainter_;
    delete viewwin_;
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva, bool isvert )
{
    const bool isnew = !viewwin_;
    if ( isnew )
	createViewWin( isvert );
    bool drawhorizon = false;
    mDynamicCastGet(Attrib::Flat3DDataPack*,dp3d,
	    		DPM(DataPackMgr::FlatID()).obtain(packid,true))
    if ( dp3d && dp3d->isVertical() )
    {
	horpainter_->setCubeSampling( dp3d->cube().cubeSampling(), true );
	drawhorizon = true;
    }
    else
    {
	mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2d,
			DPM(DataPackMgr::FlatID()).obtain(packid,true))
	if( dp2d && dp2d->isVertical() )
	{
	    horpainter_->setCubeSampling( dp2d->dataholder().getCubeSampling(),
		    			  true );
	    horpainter_->set2D( true );
	    dp2d->getPosDataTable( horpainter_->getTrcNos(), 
		    		   horpainter_->getDistances() );
	    drawhorizon = true;
	}
    }

    if ( drawhorizon )
	drawHorizons();
    
    viewwin_->viewer().setPack( wva, packid, true, !isnew );
    FlatView::DataDispPars& ddp = viewwin_->viewer().appearance().ddpars_;
    (wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;
    viewwin_->start();
}


void uiODViewer2D::createViewWin( bool isvert )
{    
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true) );
	fvmw->windowClosed.notify( mCB(this,uiODViewer2D,winCloseCB) );
	viewwin_ = fvmw;
    }
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 600, 400 );
    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer( ivwr );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
	if ( ivwr == 0 )
	{
	    viewwin_->addControl( new uiFlatViewStdControl( vwr,
		uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0") ) );
	    horpainter_ = new EM::HorizonPainter( vwr );
	    auxdataeditor_ = new uiFlatViewAuxDataEditor( vwr );
	    horfveditor_ = new MPE::HorizonFlatViewEditor( auxdataeditor_ );
	}
    }
}


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    mDynamicCastGet(uiMainWin*,mw,cb)
    if ( mw ) mw->windowClosed.remove( mCB(this,uiODViewer2D,winCloseCB) );
    viewwin_ = 0;
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as )
{
    selspec_ = as ? *as : Attrib::SelSpec();
}


void uiODViewer2D::drawHorizons()
{
    for ( int idx=0; idx<EM::EMM().nrLoadedObjects(); idx++ )
	horpainter_->addHorizon( EM::EMM().objectID(idx) );
}
