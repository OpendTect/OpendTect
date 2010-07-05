/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.11 2010-07-05 16:08:07 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratunitrepos.h"
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
    drawer_.xAxis()->setBounds( StepInterval<float>( 0, 100, 10 ) );
   
    uidatagather_.newtreeRead.notify( mCB(this,uiWellStratDisplay,dataChanged));
    dataChanged(0);
}


void uiWellStratDisplay::dataChanged( CallBacker* )
{
    for ( int colidx=0; colidx<nrCols(); colidx++ )
    {
	for ( int idx=0; idx<nrUnits( colidx )-1; idx++ )
	{
	    AnnotData::Unit* cunit = getUnit( idx, colidx );
	    AnnotData::Unit* nunit = getUnit( idx+1, colidx );
	    if ( cunit && nunit )
		setUnitPos( *cunit, *nunit );
	}
    }
    setZRange( Interval<float>( (dispData().zrg_.stop/1000), 
				(dispData().zrg_.start )/1000) );
}


void uiWellStratDisplay::setUnitPos( AnnotData::Unit& cunit, 
					AnnotData::Unit& nunit )  
{
    if ( !dispdata_.markers_ ) return;
    float& ctoppos = cunit.zpos_; 
    float& cbotpos = cunit.zposbot_; 
    float& ntoppos = nunit.zpos_;
    float& nbotpos = nunit.zposbot_;
    const Well::Marker* topmrk = 0;
    const Well::Marker* basemrk = 0;
    for ( int idx=0; idx<dispdata_.markers_->size(); idx++ )
    {
	const char* lvlnm = 
	    Strat::RT().getUnitLvlName( (*dispdata_.markers_)[idx]->levelID() );
	if ( lvlnm && !strcmp( lvlnm, cunit.annots_[0]->buf() ) )
	    topmrk = (*dispdata_.markers_)[idx];
	if ( lvlnm && !strcmp( lvlnm, nunit.annots_[0]->buf() ) )
	    basemrk = (*dispdata_.markers_)[idx];
    }
    if ( !topmrk || !basemrk ) 
    { 
	ntoppos = mUdf(float); 
	cbotpos = mUdf(float); 
    }
    else
    {
	ctoppos = topmrk->dah();
	cbotpos = basemrk->dah();
	if ( dispdata_.zistime_ && dispdata_.d2tm_ ) 
	{ 
	    ctoppos = dispdata_.d2tm_->getTime( ctoppos ); 
	    cbotpos = dispdata_.d2tm_->getTime( cbotpos ); 
	}
	ntoppos = cbotpos;
    }
}

