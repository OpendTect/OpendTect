/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimapperrangeeditordlg.h"

#include "uibutton.h"
#include "uicoltabtools.h"
#include "uimapperrangeeditor.h"
#include "uiseparator.h"
#include "uistatsdisplaywin.h"
#include "uitoolbar.h"

#include "bufstringset.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"

uiMultiMapperRangeEditWin::uiMultiMapperRangeEditWin( uiParent* p, int nr,
       						DataPackMgr::ID dmid )
    : uiDialog( p,uiDialog::Setup("Histogram",
				  mNoDlgTitle,"50.0.12").modal(false)
							.menubar(true) )
    , activeattrbid_(-1)
    , activectbmapper_(0)	      
    , rangeChange(this)
    , dpm_(DPM(dmid))		       
{
    setCtrlStyle( LeaveOnly );

    datapackids_.setSize( nr );
    uiSeparator* sephor = 0;

    uiToolBar* tb = new uiToolBar( this, "Stats" );
    tb->addButton( "info", "Statistics",
	    	   mCB(this,uiMultiMapperRangeEditWin,showStatDlg) );

    for ( int idx=0; idx<nr; idx++ )
    {
	uiMapperRangeEditor* rangeeditor =
	    			new uiMapperRangeEditor( this, idx );
	rangeeditor->rangeChanged.notify(
		mCB(this,uiMultiMapperRangeEditWin,rangeChanged) );
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
		datapackids_[idx], dpm_.id() );
	datanms.add(  DPM(dpm_.id()).nameOf(datapackids_[idx]) );
    }

    statswin->addDataNames( datanms );
    statswin->show();
}


uiMapperRangeEditor* uiMultiMapperRangeEditWin::getuiMapperRangeEditor( int idx)
{ return mapperrgeditors_.validIdx(idx) ? mapperrgeditors_[idx] : 0; }


void uiMultiMapperRangeEditWin::setDataPackID( int idx, DataPack::ID dpid )
{
    if ( !mapperrgeditors_.validIdx(idx) )
	return;

    mapperrgeditors_[idx]->setDataPackID( dpid, dpm_.id() );
    if ( datapackids_.validIdx(idx) )
	datapackids_[idx] = dpid;
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


void uiMultiMapperRangeEditWin::rangeChanged( CallBacker* cb )
{
    mDynamicCastGet(uiMapperRangeEditor*,obj,cb);
    activeattrbid_ = obj->ID();
    activectbmapper_ = &obj->getColTabMapperSetup();
    rangeChange.trigger();
}


void uiMultiMapperRangeEditWin::dataPackDeleted( CallBacker* cb )
{
    mDynamicCastGet(DataPack*,obj,cb);
    const int dpidx = datapackids_.indexOf( obj->id() );
    if ( !mapperrgeditors_.validIdx(dpidx) ) return;

    mapperrgeditors_[dpidx]->display( false );
}
