/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uimapperrangeeditordlg.h"

#include "uibutton.h"
#include "uihistogramdisplay.h"
#include "uimapperrangeeditor.h"
#include "uiseparator.h"
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
						DataPackMgr::ID dmid )
    : uiDialog( p,uiDialog::Setup(uiStrings::sHistogram(),
				mNoDlgTitle,
				mODHelpKey(mMultiMapperRangeEditWinHelpID) )
				.nrstatusflds(1)
				.modal(false)
				.menubar(true) )
    , activeattrbid_(-1)
    , activectbmapper_(0)
    , rangeChange(this)
    , dpm_(DPM(dmid))
{
    setCtrlStyle( CloseOnly );
    showAlwaysOnTop();

    datapackids_.setSize( nr, DataPack::ID::getInvalid() );
    uiSeparator* sephor = 0;

    const bool withstatsdlg = nr > 2;
    if ( withstatsdlg )
    {
	uiToolBar* tb = new uiToolBar( this, uiStrings::sStatistics() );
	tb->addButton( "info", uiStrings::sStatistics(),
		       mCB(this,uiMultiMapperRangeEditWin,showStatDlg) );
    }

    for ( int idx=0; idx<nr; idx++ )
    {
	uiMapperRangeEditor* rangeeditor =
				new uiMapperRangeEditor( this, idx );
	rangeeditor->getDisplay().getMouseEventHandler().movement.notify(
			mCB(this,uiMultiMapperRangeEditWin,mouseMoveCB) );
	mapperrgeditors_ += rangeeditor;

	if ( idx%2 == 0 )
	{
	    if ( sephor )
	    {
		rangeeditor->attach( ensureBelow, sephor );
		rangeeditor->attach( centeredBelow, mapperrgeditors_[idx-2] );
	    }
	    sephor = new uiSeparator( this, "H sep" );
	    sephor->attach( stretchedBelow, rangeeditor );
	}
	else
	    rangeeditor->attach( centeredRightOf, mapperrgeditors_[idx-1] );

	if ( !withstatsdlg )
	{
	    uiStatsDisplay::Setup sds; sds.withplot(false).withname(false);
	    uiStatsDisplay* sd = new uiStatsDisplay( this, sds );
	    statsdisplays_ += sd;
	    sd->attach( alignedBelow, rangeeditor );
	    sd->attach( ensureBelow, sephor );
	}
    }

    mAttachCB( dpm_.packToBeRemoved,
	       uiMultiMapperRangeEditWin::dataPackDeleted );
}


uiMultiMapperRangeEditWin::~uiMultiMapperRangeEditWin()
{
    detachAllNotifiers();
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
	datanms.add( DPM(dpm_.id()).nameOf(datapackids_[idx]) );
    }

    statswin->addDataNames( datanms );
    statswin->show();
}


uiMapperRangeEditor* uiMultiMapperRangeEditWin::getuiMapperRangeEditor( int idx)
{
    return mapperrgeditors_.validIdx(idx) ? mapperrgeditors_[idx] : 0;
}


void uiMultiMapperRangeEditWin::setDataPackID(
		int idx, DataPack::ID dpid, int version )
{
    if ( !mapperrgeditors_.validIdx(idx) )
	return;

    mapperrgeditors_[idx]->setDataPackID( dpid, dpm_.id(), version );
    if ( datapackids_.validIdx(idx) )
	datapackids_[idx] = dpid;

    if ( statsdisplays_.validIdx(idx) )
	statsdisplays_[idx]->setDataPackID( dpid, dpm_.id(), version );
}


void uiMultiMapperRangeEditWin::setColTabMapper( int idx,
						 const ColTab::Mapper& mpr )
{
    if ( mapperrgeditors_.validIdx(idx) )
	mapperrgeditors_[idx]->setMapper( const_cast<ColTab::Mapper&>(mpr) );
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
	    val = disp.getFuncXY( pos.x_, false );
	    break;
	}
    }

    uiString str = tr("Value / Count: %1 / %2").arg(toUiString(val.x_,4)).
		   arg(toUiString(val.y_,0));
    toStatusBar( str );
}


void uiMultiMapperRangeEditWin::rangeChanged( CallBacker* cb )
{
    mDynamicCastGet(uiMapperRangeEditor*,obj,cb);
    activeattrbid_ = obj->ID();
    activectbmapper_ = &obj->mapper();
    rangeChange.trigger();
}


void uiMultiMapperRangeEditWin::dataPackDeleted( CallBacker* cb )
{
    mDynamicCastGet(DataPack*,obj,cb);
    const int dpidx = datapackids_.indexOf( obj->id() );
    if ( !mapperrgeditors_.validIdx(dpidx) ) return;

    mapperrgeditors_[dpidx]->display( false );
}
