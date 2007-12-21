/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Nov 2007
 RCS:           $Id: uiseistrcbufviewer.cc,v 1.10 2007-12-21 12:37:35 cvssatyaki Exp $
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
{
    viewer().setInitialSize( uiSize(400,500) );
    viewer().setDarkBG( false );
    FlatView::Appearance& app = viewer().appearance();
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.wva_.overlap_ = 1;
    addControl( new uiFlatViewStdControl( viewer(),
		uiFlatViewStdControl::Setup(this).withstates(true) ) );
}


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
    cleanUp();
}


SeisTrcBufDataPack* uiSeisTrcBufViewer::setTrcBuf( SeisTrcBuf* tbuf,
				Seis::GeomType geom, const char* category,
				const char* dpname )
{
    const int type =  tbuf->get(0)->info().getDefaultAxisFld( geom,
	    		&tbuf->get(1)->info() );
    SeisTrcBufDataPack* dp =
	new SeisTrcBufDataPack( tbuf, geom, (SeisTrcInfo::Fld)type, category );
    dp->setName( dpname );
    DPM( DataPackMgr::FlatID ).add( dp );
    viewer().addPack( dp->id() );
    return dp;
}


SeisTrcBufDataPack* uiSeisTrcBufViewer::setTrcBuf( const SeisTrcBuf& tbuf,
				Seis::GeomType geom, const char* category,
				const char* dpname )
{
    SeisTrcBuf* mybuf = new SeisTrcBuf( tbuf );
    const int type =  mybuf->get(0)->info().getDefaultAxisFld( geom,
	    	    	&mybuf->get(1)->info() );

    SeisTrcBufDataPack* dp =
	new SeisTrcBufDataPack( mybuf, geom, (SeisTrcInfo::Fld)type, category );
    dp->setName( dpname );
    DPM( DataPackMgr::FlatID ).add( dp );
    viewer().addPack( dp->id() );
    return dp;
}


void uiSeisTrcBufViewer::handleBufChange()
{
    viewer().handleChange( FlatView::Viewer::All );
}


void uiSeisTrcBufViewer::removePacks()
{
    const TypeSet<DataPack::ID> ids = viewer().availablePacks();
    for ( int idx=0; idx<ids.size(); idx++ )
	viewer().removePack( ids[idx] );
}
