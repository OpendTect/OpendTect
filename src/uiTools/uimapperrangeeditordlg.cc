/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimapperrangeeditordlg.h"

#include "uibutton.h"
#include "uicoltabtools.h"
#include "uihistogramdisplay.h"
#include "uimapperrangeeditor.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uitoolbar.h"

#include "bufstringset.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"
#include "mouseevent.h"
#include "od_helpids.h"

uiMultiMapperRangeEditWin::uiMultiMapperRangeEditWin( uiParent* p, int nr,
						DataPackMgr::MgrID dmid )
    : uiDialog( p,uiDialog::Setup(uiStrings::sHistogram(),
				mNoDlgTitle,
				mODHelpKey(mMultiMapperRangeEditWinHelpID) )
				.nrstatusflds(1)
				.modal(false)
				.menubar(true) )
    , activeattrbid_(-1)
    , activectbmapper_(0)
    , activectbseq_(0)
    , rangeChange(this)
    , sequenceChange(this)
    , dpm_(DPM(dmid))
{
    setCtrlStyle( CloseOnly );

    datapackids_.setSize( nr );

    // Assuming max number of texture layers is 8
    const int nrcols = nr < 5 ? nr : (nr<7 ? 3 : 4);
    const int nrrows = nr < 5 ? 1  : 2;
    const int total = nrcols * nrrows;
    const bool withstatsdlg = nrrows == 2;
    if ( withstatsdlg )
    {
	uiToolBar* tb = new uiToolBar( this, tr("Stats") );
	tb->addButton( "info", tr("Statistics"),
		       mCB(this,uiMultiMapperRangeEditWin,showStatDlg) );
    }

    for ( int idx=0; idx<total; idx++ )
    {
	auto* rangeeditor = new uiMapperRangeEditor( this, idx );
	rangeeditor->rangeChanged.notify(
		mCB(this,uiMultiMapperRangeEditWin,rangeChanged) );
	rangeeditor->sequenceChanged.notify(
		mCB(this,uiMultiMapperRangeEditWin,sequenceChanged) );
	rangeeditor->getDisplay().getMouseEventHandler().movement.notify(
			mCB(this,uiMultiMapperRangeEditWin,mouseMoveCB) );
	mapperrgeditors_ += rangeeditor;

	if ( idx==nrcols )
	    rangeeditor->attach( centeredBelow, mapperrgeditors_[idx-nrcols] );
	else if ( idx>0 )
	    rangeeditor->attach( centeredRightOf, mapperrgeditors_[idx-1] );

	if ( !withstatsdlg )
	{
	    uiStatsDisplay::Setup sds; sds.withplot(false).withname(false);
	    uiStatsDisplay* sd = new uiStatsDisplay( rangeeditor, sds );
	    statsdisplays_ += sd;
	    sd->attach( alignedBelow, &rangeeditor->getDisplay() );
	}

	rangeeditor->display( idx<nr );
    }

    dpm_.packToBeRemoved.notifyIfNotNotified(
			mCB(this,uiMultiMapperRangeEditWin,dataPackDeleted) );
}


uiMultiMapperRangeEditWin::~uiMultiMapperRangeEditWin()
{
    dpm_.packToBeRemoved.remove(
			mCB(this,uiMultiMapperRangeEditWin,dataPackDeleted) );
}


void uiMultiMapperRangeEditWin::showStatDlg( CallBacker* )
{
    uiStatsDisplayWin* statswin = new uiStatsDisplayWin( this,
	    uiStatsDisplay::Setup().withplot(false).withname(false),
	    datapackids_.size(), false );
    statswin->setDeleteOnClose( false );
    BufferStringSet datanms;
    for ( int idx=0; idx<datapackids_.size(); idx++ )
    {
	statswin->statsDisplay( idx )->setDataPackID(
				datapackids_[idx], dpm_.id(), 0 );
	datanms.add(  DPM(dpm_.id()).nameOf(datapackids_[idx]) );
    }

    statswin->addDataNames( datanms );
    statswin->show();
}


uiMapperRangeEditor* uiMultiMapperRangeEditWin::getuiMapperRangeEditor( int idx)
{ return mapperrgeditors_.validIdx(idx) ? mapperrgeditors_[idx] : 0; }


void uiMultiMapperRangeEditWin::setDataPackID(
		int idx, DataPackID dpid, int version )
{
    if ( !mapperrgeditors_.validIdx(idx) )
	return;

    mapperrgeditors_[idx]->setDataPackID( dpid, dpm_.id(), version );
    mapperrgeditors_[idx]->display( true );
    if ( datapackids_.validIdx(idx) )
	datapackids_[idx] = dpid;

    if ( statsdisplays_.validIdx(idx) )
	statsdisplays_[idx]->setDataPackID( dpid, dpm_.id(), version );
}


void uiMultiMapperRangeEditWin::setColTabMapperSetup( int idx,
						const ColTab::MapperSetup& ms )
{
    if ( !mapperrgeditors_.validIdx(idx) )
	return;

    mapperrgeditors_[idx]->setColTabMapperSetup( ms );
}


void uiMultiMapperRangeEditWin::setColTabSeq( int idx,
						const ColTab::Sequence& ctseq )
{
    if ( !mapperrgeditors_.validIdx(idx) )
	return;

    mapperrgeditors_[idx]->setColTabSeq( ctseq );
}


void uiMultiMapperRangeEditWin::setActiveAttribID( int id )
{
    if ( mapperrgeditors_.isEmpty() || mapperrgeditors_.size() > 1 )
	return;

    mapperrgeditors_[0]->setID( id );
}


void uiMultiMapperRangeEditWin::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb)
    if ( !meh ) return;

    Geom::Point2D<float> val;
    const Geom::Point2D<int>& pos = meh->event().pos();
    for ( int idx=0; idx<mapperrgeditors_.size(); idx++ )
    {
	uiHistogramDisplay& disp = mapperrgeditors_[idx]->getDisplay();
	if ( &disp.getMouseEventHandler() == meh )
	{
	    val = disp.getFuncXY( pos.x, false );
	    break;
	}
    }

    uiString str = tr("Value / Count:  %1 / %2").arg(toUiString(val.x,4)).
		   arg(toUiString(val.y,0));
    toStatusBar( str );
}


void uiMultiMapperRangeEditWin::rangeChanged( CallBacker* cb )
{
    mDynamicCastGet(uiMapperRangeEditor*,obj,cb);
    activeattrbid_ = obj->ID();
    activectbmapper_ = &obj->getColTabMapperSetup();
    rangeChange.trigger();
}


void uiMultiMapperRangeEditWin::sequenceChanged( CallBacker* cb )
{
    mDynamicCastGet(uiMapperRangeEditor*,obj,cb);
    activeattrbid_ = obj->ID();
    activectbseq_ = &obj->getColTabSeq();
    sequenceChange.trigger();
}


void uiMultiMapperRangeEditWin::dataPackDeleted( CallBacker* cb )
{
    mDynamicCastGet(DataPack*,obj,cb);
    const int dpidx = datapackids_.indexOf( obj->id() );
    if ( !mapperrgeditors_.validIdx(dpidx) ) return;

    mapperrgeditors_[dpidx]->display( false );
}
