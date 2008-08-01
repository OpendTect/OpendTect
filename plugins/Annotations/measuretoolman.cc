/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2008
 RCS:		$Id: measuretoolman.cc,v 1.1 2008-08-01 12:12:38 cvsnanne Exp $
________________________________________________________________________

-*/


#include "measuretoolman.h"

#include "uimeasuredlg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "vismeasure.h"
#include "visselman.h"

#include "draw.h"

namespace Annotations
{

MeasureToolMan::MeasureToolMan( uiODMain& appl )
    : appl_(appl)
    , measuredlg_(0)
{
    const CallBack cb( mCB(this,MeasureToolMan,buttonClicked) );
    appl.menuMgr().coinTB()->addButton( "measure.png", cb, 
	    				"Display Distance", true );

    appl.sceneMgr().treeToBeAdded.notify(
			mCB(this,MeasureToolMan,sceneAdded) );
    appl.sceneMgr().sceneClosed.notify(
			mCB(this,MeasureToolMan,sceneClosed) );
}


void MeasureToolMan::buttonClicked( CallBacker* cb )
{
    appl_.sceneMgr().setToViewMode( false );

    for ( int idx=0; idx<measureobjs_.size(); idx++ )
    {
	if ( true )
	{
	    visBase::DM().selMan().select( measureobjs_[idx]->id() );
	}
	else
	{
	    visBase::DM().selMan().deSelect( measureobjs_[idx]->id() );
//	    measureobjs_[idx]->removePolyline();
//	    measuredlg_->reset();
	}

    }
}


void MeasureToolMan::sceneAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    MeasureDisplay* md = MeasureDisplay::create();
    md->ref();
    md->changed.notify( mCB(this,MeasureToolMan,changeCB) );
    appl_.applMgr().visServer()->addObject( md, sceneid, false );

    sceneids_ += sceneid;
    measureobjs_ += md;
}


void MeasureToolMan::sceneClosed( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    const int sceneidx = sceneids_.indexOf( sceneid );
    if ( sceneidx<0 || sceneidx>=measureobjs_.size() )
	return;

    appl_.applMgr().visServer()->removeObject( measureobjs_[sceneidx], sceneid);
    measureobjs_[sceneidx]->unRef();
    measureobjs_.remove( sceneidx );
} 


void MeasureToolMan::changeCB( CallBacker* cb )
{
    mDynamicCastGet(MeasureDisplay*,md,cb);
    if ( !md ) return;

    if ( !measuredlg_ )
    {
	measuredlg_ = new uiMeasureDlg( 0 );
	measuredlg_->propertyChange.notify(
				mCB(this,MeasureToolMan,propChangeCB) ) ;
	measuredlg_->clearPressed.notify( mCB(this,MeasureToolMan,clearCB) );
    }

    measuredlg_->show();
    TypeSet<Coord3> crds;
    md->getPoints( crds );
    measuredlg_->fill( crds );
}


void MeasureToolMan::clearCB( CallBacker* )
{
    for ( int idx=0; idx<measureobjs_.size(); idx++ )
	measureobjs_[idx]->removePolyline();

    measuredlg_->reset();
}


void MeasureToolMan::propChangeCB( CallBacker* cb )
{
    for ( int idx=0; idx<measureobjs_.size(); idx++ )
    {
      	LineStyle ls( measuredlg_->getLineStyle() );
    	measureobjs_[idx]->setLineStyle( ls );
    }
}


} // namespace Annotations
