/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Nov 2007
 RCS:           $Id: uiseistrcbufviewer.cc,v 1.2 2007-11-21 09:51:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseistrcbufviewer.h"
#include "seisbufadapters.h"
#include "seisinfo.h"
#include "seisinfo.h"
#include "uiflatviewer.h"

uiSeisTrcBufViewer::uiSeisTrcBufViewer( uiParent* p, 
	uiSeisTrcBufViewer::Setup& setup,Seis::GeomType geom, SeisTrcBuf* tbuf )
    : uiFlatViewMainWin( p, setup )      
    , geom_(geom)
    , app_(0)		 
    , vwr_(0)		 
    , viewwin_(0)		 
    , setup_(setup)		 
{
    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( tbuf, Seis::VolPS, 
	                         SeisTrcInfo::TrcNr, "" );
    DPM( DataPackMgr::FlatID ).add( dp );
    initialise();
    setData( dp );
}


uiSeisTrcBufViewer::uiSeisTrcBufViewer(uiParent* p, 
		    uiSeisTrcBufViewer::Setup& setup,
		    Seis::GeomType geom, FlatDataPack* dp )
    : uiFlatViewMainWin( p, setup )      
    , geom_(geom)
    , app_(0)		 
    , vwr_(0)		 
    , viewwin_(0)		 
    , setup_(setup)		 
{
    initialise();
    DataPack::ID dpid = dp->id();
    setData( dp );
}		 


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
    delete app_;
    delete vwr_;
}


void uiSeisTrcBufViewer::initialise()
{
    viewwin_ = new uiFlatViewMainWin( this, 
                            uiFlatViewMainWin::Setup(setup_.wintitle_) );
    viewwin_->setDarkBG( false );
    app_ = new FlatView::Appearance(viewwin_->viewer().appearance());
    app_->annot_.setAxesAnnot( true );
    app_->setGeoDefaults( true );
    app_->ddpars_.wva_.overlap_ = 1;
    vwr_ = new uiFlatViewer(viewwin_->viewer());
}


void uiSeisTrcBufViewer::setData( FlatDataPack* dp )
{
    vwr_->setPack( true, dp );
    vwr_->appearance().ddpars_.show( true, false );
    viewwin_->start();
}
