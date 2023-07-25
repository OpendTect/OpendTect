/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseistrcbufviewer.h"

#include "seisbufadapters.h"
#include "seisinfo.h"
#include "seistrc.h"

#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"


uiSeisTrcBufViewer::uiSeisTrcBufViewer( uiParent* p,
					const uiFlatViewMainWin::Setup& setup )
    : uiFlatViewMainWin( p, setup )
    , dp_(nullptr)
{
    viewer().setInitialSize( uiSize(420,600) );
    FlatView::Appearance& app = viewer().appearance();
    app.setDarkBG( false );
    app.annot_.setAxesAnnot( true );
    app.setGeoDefaults( true );
    app.ddpars_.wva_.overlap_ = 1;
    addControl( new uiFlatViewStdControl( viewer(),
		uiFlatViewStdControl::Setup(this) ) );
}


uiSeisTrcBufViewer::~uiSeisTrcBufViewer()
{
    releaseDP();
}


void uiSeisTrcBufViewer::releaseDP()
{
    dp_ = nullptr;
}


void uiSeisTrcBufViewer::setBuf( const SeisTrcBuf& tbuf,
				 Seis::GeomType geom, const char* cat,
				 const char* dpname, int compnr, bool mine )
{
    releaseDP();
    if ( tbuf.isEmpty() ) return;

    const int sz = tbuf.size();
    SeisTrcInfo::Fld type = SeisTrcInfo::SeqNr;
    if ( sz > 1 )
    {
	const SeisTrcInfo& first = tbuf.first()->info();
	const SeisTrcInfo& next = tbuf.get(1)->info();
	const SeisTrcInfo& last = tbuf.last()->info();
	type = first.getDefaultAxisFld( geom, &next, &last );
    }

    if ( mine )
	dp_ = new SeisTrcBufDataPack( const_cast<SeisTrcBuf*>(&tbuf), geom,
				     type, cat, compnr );
    else
	dp_ = new SeisTrcBufDataPack( tbuf, geom, type, cat, compnr );
    dp_->setName( dpname );

    DPM( DataPackMgr::FlatID() ).add( dp_ );
    const FlatView::DataDispPars& ddpars = viewer().appearance().ddpars_;
    viewer().addPack( dp_->id() );
    const FlatView::Viewer::VwrDest dest =
	  FlatView::Viewer::getDest( ddpars.wva_.show_, ddpars.vd_.show_ );
    if ( dest != FlatView::Viewer::None )
	viewer().usePack( dest, dp_->id() );

    int w = 200 + 2*sz; if ( w > 600 ) w = 600;
    int h = 150 + 5*tbuf.first()->size(); if ( h > 500 ) h = 500;
    viewer().setInitialSize( uiSize(w,h) );
}


void uiSeisTrcBufViewer::setTrcBuf( const SeisTrcBuf* tbuf,
				Seis::GeomType geom, const char* cat,
				const char* dpname, int compnr )
{
    if ( tbuf )
	setBuf( *tbuf, geom, cat, dpname, compnr, false );
}


void uiSeisTrcBufViewer::setTrcBuf( const SeisTrcBuf& tbuf,
				Seis::GeomType geom, const char* cat,
				const char* dpname, int compnr )
{
    setBuf( *new SeisTrcBuf(tbuf), geom, cat, dpname, compnr, true );
}


void uiSeisTrcBufViewer::selectDispTypes( bool wva, bool vd )
{
    viewer().appearance().ddpars_.show( wva, vd );
}


void uiSeisTrcBufViewer::handleBufChange()
{
    viewer().handleChange( sCast(od_uint32,FlatView::Viewer::All) );
}
