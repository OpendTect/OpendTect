/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.cc,v 1.1 2008-12-30 09:08:50 cvsumesh Exp $
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
    uiSeparator* sep = 0;
    for ( int idx=0; idx<n; idx++ )
    {
	uiMapperRangeEditor* rangeeditor =
	    			new uiMapperRangeEditor( this, idx );
	mapperrgeditors_ += rangeeditor;
	if ( sep )
	    rangeeditor->attach( ensureBelow, sep );
	rangeeditor->rangeChanged.notify( mCB(this,uiMultiRangeSelDispWin,
		    		       	       rangeChanged) );
	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, rangeeditor );
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
