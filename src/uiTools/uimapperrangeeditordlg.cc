/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.cc,v 1.2 2009-01-08 10:14:40 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uimultirangeseldisplaywin.h"

#include "uicoltabtools.h"
#include "uimapperrangeeditor.h"
#include "uiseparator.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"

uiMultiRangeSelDispWin::uiMultiRangeSelDispWin( uiParent* p, int n )
    : uiDialog( p,uiDialog::Setup("Histogram",
				   mNoDlgTitle,mTODOHelpID).modal(false) )
    , activeattrbid_(-1)
    , activectbmapper_(0)	      
    , rangeChange(this)			
{
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
    
    setOkText( "Dismiss" );
    setCancelText( "" );
}


uiMapperRangeEditor* uiMultiRangeSelDispWin::getuiMapperRangeEditor( int idx )
{
    return mapperrgeditors_[idx];
}


void uiMultiRangeSelDispWin::setDataPackID( int idx, DataPack::ID dpid,
					    DataPackMgr::ID dmid )
{
    mapperrgeditors_[idx]->setDataPackID( dpid, dmid );
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
