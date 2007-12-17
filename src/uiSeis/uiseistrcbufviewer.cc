/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Nov 2007
<<<<<<< uiseistrcbufviewer.cc
 RCS:           $Id: uiseistrcbufviewer.cc,v 1.8 2007-12-17 05:53:18 cvssatyaki Exp $
=======
 RCS:           $Id: uiseistrcbufviewer.cc,v 1.8 2007-12-17 05:53:18 cvssatyaki Exp $
>>>>>>> 1.7
________________________________________________________________________

-*/

#include "seisbufadapters.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uirgbarraycanvas.h"
#include "uiseistrcbufviewer.h"


uiSeisTrcBufViewer::uiSeisTrcBufViewer( uiParent* p, 
					const uiSeisTrcBufViewer::Setup& setup )
    : uiFlatViewMainWin( p, setup )      
    , vwr_(0)		 
{
    vwr_ = new uiFlatViewer(this);
    vwr_->setInitialSize( uiSize(400,500) );
    vwr_->setDarkBG( false );
    FlatView::Appearance& app = vwr_->appearance();
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.wva_.overlap_ = 1;
    addControl( new uiFlatViewStdControl( *vwr_, 
		uiFlatViewStdControl::Setup(this).withstates(true) ) );
}


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
    delete vwr_;
}


// TODO: look at nr of arguments
SeisTrcBufDataPack* uiSeisTrcBufViewer::setTrcBuf( SeisTrcBuf* tbuf,
				Seis::GeomType geom, SeisTrcInfo::Fld fld,
				const char* category, const char* nm )
{
    SeisTrcBufDataPack* dp =
	new SeisTrcBufDataPack( tbuf, geom, fld, category );
    dp->setName( nm );
    DPM( DataPackMgr::FlatID ).add( dp );
    vwr_->addPack( dp->id() );
    return dp;
}


void uiSeisTrcBufViewer::update()
{
    vwr_->handleChange( FlatView::Viewer::All );
}
