/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.10 2010-06-24 11:54:01 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratlevel.h"
#include "uigraphicsscene.h"
#include "uistrattreewin.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"

uiWellStratDisplay::uiWellStratDisplay( uiParent* p, bool nobg,
					const Well::Well2DDispData& dd)
    : uiAnnotDisplay(p,"")
    , dispdata_(dd)
    , uidatagather_(uiStratAnnotGather(data_,StratTWin().mgr()))
{
    if ( nobg )
    {
	setNoSytemBackGroundAttribute();
	uisetBackgroundColor( Color( 255, 255, 255, 255 )  );
	scene().setBackGroundColor( Color( 255, 255, 255, 255 )  );
    }
    drawer_.setNewAxis( new uiAxisHandler(scene_,
					  uiAxisHandler::Setup(uiRect::Left)
					  .noborderspace(true)
					  .border(uiBorder(0))
					  .nogridline(true)), false );
    drawer_.setNewAxis( new uiAxisHandler(scene_,
	    				 uiAxisHandler::Setup(uiRect::Top)
					 .noborderspace(true)
					 .border(uiBorder(0))
					 .nogridline(true) ), true );
   
    uidatagather_.newtreeRead.notify( mCB(this,uiWellStratDisplay,dataChanged));
    dataChanged(0);
}


void uiWellStratDisplay::dataChanged( CallBacker* )
{
    for ( int colidx=0; colidx<nrCols(); colidx++ )
    {
	for ( int idx=0; idx<nrUnits( colidx ); idx++ )
	{
	    AnnotData::Unit* unit = getUnit( idx, colidx );
	    if ( unit )
		setUnitPos( *unit );
	}
    }
    setZRange( Interval<float>( (dispData().zrg_.stop/1000), 
				(dispData().zrg_.start )/1000) );
}


void uiWellStratDisplay::setUnitPos( AnnotData::Unit& unit )  
{
    if ( !dispdata_.markers_ ) return;
    float& toppos = unit.zpos_; 
    float& botpos = unit.zposbot_;
    const Well::Marker* topmrk = 0;
    const Well::Marker* basemrk = 0;
    for ( int idx=0; idx<dispdata_.markers_->size(); idx++ )
    {
    }
    if ( !topmrk || !basemrk ) 
    { 
	toppos = mUdf(float); 
	botpos = mUdf(float); 
    }
    else
    {
	toppos = topmrk->dah();
	botpos = basemrk->dah();
	if ( dispdata_.zistime_ && dispdata_.d2tm_ ) 
	{ 
	    toppos = dispdata_.d2tm_->getTime( toppos ); 
	    botpos = dispdata_.d2tm_->getTime( botpos ); 
	}
    }
}

