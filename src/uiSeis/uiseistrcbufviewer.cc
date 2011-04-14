/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Nov 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseistrcbufviewer.cc,v 1.20 2011-04-14 13:50:04 cvsbruno Exp $";

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
    viewer().setInitialSize( uiSize(420,600) );
    FlatView::Appearance& app = viewer().appearance();
    app.setDarkBG( false );
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.wva_.overlap_ = 1;
    addControl( new uiFlatViewStdControl( viewer(),
		uiFlatViewStdControl::Setup(this).withstates(true) ) );
}


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
}


SeisTrcBufDataPack* uiSeisTrcBufViewer::setTrcBuf( SeisTrcBuf* tbuf,
				Seis::GeomType geom, const char* category,
				const char* dpname, int compnr )
{
    if ( !tbuf ) return 0;
    const int sz = tbuf->size();
    if ( sz < 1 ) return 0;
    const int type = sz < 2 ? (int)SeisTrcInfo::TrcNr
	: tbuf->get(0)->info().getDefaultAxisFld( geom, &tbuf->get(1)->info() );

    SeisTrcBufDataPack* dp =
	new SeisTrcBufDataPack( tbuf, geom, (SeisTrcInfo::Fld)type,
				category, compnr );
    dp->setName( dpname );
    DPM( DataPackMgr::FlatID() ).add( dp );
    viewer().addPack( dp->id() );

    int w = 250 + sz; if ( w > 600 ) w = 600;
    viewer().setInitialSize( uiSize(w,500) );
    return dp;
}


SeisTrcBufDataPack* uiSeisTrcBufViewer::setTrcBuf( const SeisTrcBuf& tbuf,
				Seis::GeomType geom, const char* category,
				const char* dpname, int compnr )
{
    return setTrcBuf( new SeisTrcBuf( tbuf ), geom, category, dpname, compnr );
}


void uiSeisTrcBufViewer::handleBufChange()
{
    viewer().handleChange( FlatView::Viewer::All );
}


void uiSeisTrcBufViewer::clearData()
{
    const TypeSet<DataPack::ID> ids = viewer().availablePacks();
    for ( int idx=0; idx<ids.size(); idx++ )
	viewer().removePack( ids[idx] );
}
