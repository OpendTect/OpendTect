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

uiMultiMapperRangeEditWin::uiMultiMapperRangeEditWin( uiParent* p, int nr )
    : uiDialog( p,uiDialog::Setup(uiStrings::sHistogram(),
				mNoDlgTitle,
				mODHelpKey(mMultiMapperRangeEditWinHelpID) )
				.nrstatusflds(1)
				.modal(false)
				.menubar(true) )
    , rangeChange(this)
    , sequenceChange(this)
{
    setCtrlStyle( CloseOnly );

    // Assuming max number of texture layers is 8
    const int nrcols = nr < 5 ? nr : (nr<7 ? 3 : 4);
    const int nrrows = nr < 5 ? 1  : 2;
    const int total = nrcols * nrrows;
    const bool withstatsdlg = nrrows == 2;
    if ( withstatsdlg )
    {
	auto* tb = new uiToolBar( this, tr("Stats") );
	tb->addButton( "info", tr("Statistics"),
		       mCB(this,uiMultiMapperRangeEditWin,showStatDlg) );
    }

    for ( int idx=0; idx<total; idx++ )
    {
	auto* mapperdata = new MapperData();
	auto* rangeeditor = new uiMapperRangeEditor( this, idx );
	mAttachCB( rangeeditor->rangeChanged,
			uiMultiMapperRangeEditWin::rangeChanged );
	mAttachCB( rangeeditor->sequenceChanged,
			uiMultiMapperRangeEditWin::sequenceChanged );
	mAttachCB( rangeeditor->getDisplay().getMouseEventHandler().movement,
			uiMultiMapperRangeEditWin::mouseMoveCB );
	mapperdata->mapperrgeditor_ = rangeeditor;

	if ( idx==nrcols )
	    rangeeditor->attach( centeredBelow,
				 mapperdatas_.get(idx-nrcols)->mapperrgeditor_);
	else if ( idx>0 )
	    rangeeditor->attach( centeredRightOf,
				 mapperdatas_.get(idx-1)->mapperrgeditor_ );

	if ( !withstatsdlg )
	{
	    uiStatsDisplay::Setup sds;
	    sds.withplot(false).withname(false);
	    auto* sd = new uiStatsDisplay( rangeeditor, sds );
	    sd->attach( alignedBelow, &rangeeditor->getDisplay() );
	    mapperdata->statsdisplay_ = sd;
	}

	rangeeditor->display( idx<nr );
	mapperdatas_.add( mapperdata );
    }
}


uiMultiMapperRangeEditWin::~uiMultiMapperRangeEditWin()
{
    detachAllNotifiers();
}


void uiMultiMapperRangeEditWin::showStatDlg( CallBacker* )
{
    auto* statswin = new uiStatsDisplayWin( this,
			uiStatsDisplay::Setup().withplot(false).withname(false),
			mapperdatas_.size(), false );
    statswin->setDeleteOnClose( false );
    BufferStringSet datanms; int idx=0;
    for ( const auto* mapperdata : mapperdatas_ )
    {
	ConstRefMan<DataPack> dp = mapperdata->getDataPack();
	if ( dp )
	    statswin->statsDisplay( idx++ )->setDataPack( *dp.ptr() );

	datanms.add( dp ? dp->name() : BufferString::empty() );
    }

    statswin->addDataNames( datanms );
    statswin->show();
}


uiMapperRangeEditor* uiMultiMapperRangeEditWin::getuiMapperRangeEditor( int idx)
{
    return mapperdatas_.validIdx(idx) ? mapperdatas_.get(idx)->mapperrgeditor_
				      : nullptr;
}


void uiMultiMapperRangeEditWin::setDataPackID( int idx, const DataPackID& dpid,
					       const DataPackMgr::MgrID& dmid,
					       int version )
{
    ConstRefMan<DataPack> dp = DPM( dmid ).get<DataPack>( dpid );
    return setDataPack( idx, dp.ptr(), version );
}


void uiMultiMapperRangeEditWin::setDataPack( int idx, const DataPack* dp,
					     int version )
{
    if ( !mapperdatas_.validIdx(idx) )
	return;

    MapperData& mapperdata = *mapperdatas_.get( idx );
    if ( dp )
	mapperdata.mapperrgeditor_->setDataPack( *dp, version );

    mapperdata.mapperrgeditor_->display( dp );
    mapperdata.setDataPack( dp );
    if ( mapperdata.statsdisplay_ && dp )
	mapperdata.statsdisplay_->setDataPack( *dp, version );
}


void uiMultiMapperRangeEditWin::setColTabMapperSetup( int idx,
						const ColTab::MapperSetup& ms )
{
    if ( mapperdatas_.validIdx(idx) )
	mapperdatas_.get( idx )->mapperrgeditor_->setColTabMapperSetup( ms );
}


void uiMultiMapperRangeEditWin::setColTabSeq( int idx,
					      const ColTab::Sequence& ctseq )
{
    if ( mapperdatas_.validIdx(idx) )
	mapperdatas_.get( idx )->mapperrgeditor_->setColTabSeq( ctseq );
}


void uiMultiMapperRangeEditWin::setActiveAttribID( int id )
{
    if ( mapperdatas_.size() == 1 )
	mapperdatas_.first()->mapperrgeditor_->setID( id );
}


void uiMultiMapperRangeEditWin::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb)
    if ( !meh )
	return;

    Geom::Point2D<float> val;
    const Geom::Point2D<int>& pos = meh->event().pos();
    for ( const auto* mapperdata : mapperdatas_ )
    {
	uiHistogramDisplay& disp =
				mapperdata->mapperrgeditor_->getDisplay();
	if ( &disp.getMouseEventHandler() == meh )
	{
            val = disp.getFuncXY( pos.x_, false );
	    break;
	}
    }

    const uiString str = tr("Value / Count:  %1 / %2")
				.arg(toUiString(val.x_,4))
				.arg(toUiString(val.y_,0));
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


// uiMultiMapperRangeEditWin::MapperData

uiMultiMapperRangeEditWin::MapperData::MapperData()
{}


uiMultiMapperRangeEditWin::MapperData::~MapperData()
{
    detachAllNotifiers();
}


void uiMultiMapperRangeEditWin::MapperData::setDataPack( const DataPack* dp )
{
    datapack_ = const_cast<DataPack*>( dp );
    if ( dp )
	mAttachCB( dp->objectToBeDeleted(), MapperData::dataPackDeleted );
}


ConstRefMan<DataPack> uiMultiMapperRangeEditWin::MapperData::getDataPack() const
{
    return datapack_.get();
}


void uiMultiMapperRangeEditWin::MapperData::dataPackDeleted( CallBacker* )
{
    datapack_ = nullptr;
    closeAndNullPtr( statsdisplay_ );
    mapperrgeditor_->display( false );
}
