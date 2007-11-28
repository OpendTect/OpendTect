/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Nov 2007
 RCS:           $Id: uiseistrcbufviewer.cc,v 1.3 2007-11-28 03:15:45 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiseistrcbufviewer.h"
#include "seisbufadapters.h"
#include "seisinfo.h"
#include "seisinfo.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"

uiSeisTrcBufViewer::uiSeisTrcBufViewer( uiParent* p, 
	const uiSeisTrcBufViewer::Setup& setup, Seis::GeomType geom, 
	SeisTrcBuf* tbuf )
    : uiFlatViewMainWin( p, setup )      
    , geom_(geom)
    , app_(0)		 
    , vwr_(0)		 
    , setup_(setup)		 
    , stbufsetup_(setup)		 
    , dp_(0)		 
{
    dp_ = new SeisTrcBufDataPack( tbuf, geom_, SeisTrcInfo::TrcNr, "" );
    dp_->trcBufArr2D().setBufMine( false );
    dp_->setName(stbufsetup_.nm_);
    DPM( DataPackMgr::FlatID ).add( dp_ );
    initialise();
    setData( dp_ );
}


uiSeisTrcBufViewer::uiSeisTrcBufViewer(uiParent* p, 
    const uiSeisTrcBufViewer::Setup& setup, FlatDataPack* datapack_ )
    : uiFlatViewMainWin( p, setup )      
    , app_(0)		 
    , vwr_(0)		 
    , setup_(setup)		 
    , stbufsetup_(setup)		 
    , dp_(0)		 
{
    initialise();
    DataPack::ID dpid = datapack_->id();
    setData( datapack_ );
}		 


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
    delete vwr_;
}


void uiSeisTrcBufViewer::initialise()
{
    vwr_ = new uiFlatViewer(this);
    vwr_->setInitialSize( uiSize(400,500) );
    app_ = &vwr_->appearance();
    vwr_->setDarkBG( false );
    app_->annot_.setAxesAnnot( true );
    app_->setGeoDefaults( true );
    app_->ddpars_.wva_.overlap_ = 1;
}


void uiSeisTrcBufViewer::setData( FlatDataPack* dp_ )
{
    vwr_->setPack( true, dp_ );
    app_->ddpars_.show( true, false );
    addControl( new uiFlatViewStdControl( *vwr_, 
		uiFlatViewStdControl::Setup(this).withstates(true) ) );
    start();
}
