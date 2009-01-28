/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.cc,v 1.9 2009-01-28 11:41:37 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uimapperrangeeditordlg.h"

#include "uicoltabtools.h"
#include "uimapperrangeeditor.h"
#include "uiseparator.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"

uiMultiMapperRangeEditWin::uiMultiMapperRangeEditWin( uiParent* p, int nr,
       						DataPackMgr::ID dmid )
    : uiDialog( p,uiDialog::Setup("Histogram",
				   mNoDlgTitle,mTODOHelpID).modal(false) )
    , activeattrbid_(-1)
    , activectbmapper_(0)	      
    , rangeChange(this)
    , dpm_(DPM(dmid))		       
{
    setCtrlStyle( LeaveOnly );

    datapackids_.setSize( nr );
    uiSeparator* sephor = 0;

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


uiMapperRangeEditor* uiMultiMapperRangeEditWin::getuiMapperRangeEditor( int idx )
{
    if ( idx < 0 || idx > (mapperrgeditors_.size()-1) )
	return 0;
    else 
	return mapperrgeditors_[idx];
}


void uiMultiMapperRangeEditWin::setDataPackID( int idx, DataPack::ID dpid )
{
    if ( idx < 0 || idx > (mapperrgeditors_.size()-1) )
	return;

    mapperrgeditors_[idx]->setDataPackID( dpid, dpm_.id() );
    datapackids_.insert( idx, dpid );
}


void uiMultiMapperRangeEditWin::setColTabMapperSetup( int idx,
						const ColTab::MapperSetup& ms )
{
   if ( idx < 0 || idx > (mapperrgeditors_.size()-1) )
      return;
   
   mapperrgeditors_[idx]->setColTabMapperSetup( ms ); 
}


void uiMultiMapperRangeEditWin::setColTabSeq( int idx, 
						const ColTab::Sequence& ctseq )
{
    if ( idx < 0 || idx > (mapperrgeditors_.size()-1) )
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
    if ( dpidx<0 ) return;

    mapperrgeditors_[dpidx]->setSensitive( false );
}
