/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.cc,v 1.3 2009-01-15 06:59:15 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uimultirangeseldisplaywin.h"

#include "uicoltabtools.h"
#include "uimapperrangeeditor.h"
#include "uiseparator.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"

uiMultiRangeSelDispWin::uiMultiRangeSelDispWin( uiParent* p, int n,
       						DataPackMgr::ID dmid )
    : uiDialog( p,uiDialog::Setup("Histogram",
				   mNoDlgTitle,mTODOHelpID).modal(false) )
    , activeattrbid_(-1)
    , activectbmapper_(0)	      
    , rangeChange(this)
    , dpm_(DPM(dmid))		       
{
    datapackids_.setSize( n );

    uiSeparator* sephor = 0;

    for ( int idx=0; idx<n; idx++ )
    {
	uiMapperRangeEditor* rangeeditor =
	    			new uiMapperRangeEditor( this, idx );
	rangeeditor->rangeChanged.notify( mCB(this,uiMultiRangeSelDispWin,
		    			  rangeChanged) );
	mapperrgeditors_ += rangeeditor;

	if ( (idx%2) == 0 )
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
	{
	    if ( sephor )
		rangeeditor->attach( rightAlignedAbove, sephor );
	    rangeeditor->attach( centeredRightOf, mapperrgeditors_[idx-1] );
	    if ( idx > 2 )
	    rangeeditor->attach( centeredBelow, mapperrgeditors_[idx-2] );

	}
    }
    setCtrlStyle( LeaveOnly );
    dpm_.packToBeRemoved.notifyIfNotNotified( mCB(this,uiMultiRangeSelDispWin,
					      dataPackDeleted) );
}


uiMultiRangeSelDispWin::~uiMultiRangeSelDispWin()
{
    dpm_.packToBeRemoved.remove( mCB(this,uiMultiRangeSelDispWin,
				 dataPackDeleted) );
}


uiMapperRangeEditor* uiMultiRangeSelDispWin::getuiMapperRangeEditor( int idx )
{
    return mapperrgeditors_[idx];
}


void uiMultiRangeSelDispWin::setDataPackID( int idx, DataPack::ID dpid )
{
    mapperrgeditors_[idx]->setDataPackID( dpid, dpm_.id() );
    datapackids_.insert( idx, dpid );
}


void uiMultiRangeSelDispWin::setColTabMapperSetupWthSeq( int idx,
						 const ColTab::MapperSetup& ms,
       						 const ColTab::Sequence& ctseq )
{
    mapperrgeditors_[idx]->setColTabMapperSetupWthSeq( ms, ctseq );
}


void uiMultiRangeSelDispWin::rangeChanged( CallBacker* cb)
{
    mDynamicCastGet(uiMapperRangeEditor*,obj,cb);
    activeattrbid_ = obj->ID();
    activectbmapper_ = obj->getColTabMapperSetup();
    rangeChange.trigger();
}


void uiMultiRangeSelDispWin::dataPackDeleted( CallBacker* cb )
{
    mDynamicCastGet(DataPack*,obj,cb);
    mapperrgeditors_[datapackids_.indexOf(obj->id())]->setSensitive( false );
}
