/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Oct 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiecheckshotedit.cc,v 1.1 2011-10-27 12:40:57 cvsbruno Exp $";

#include "uiwelltiecheckshotedit.h"
#include "uimsg.h"
#include "uiwelldahdisplay.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"


#define mErrRet(msg) { uiMSG().error( msg ); return; }

namespace WellTie
{
uiCheckShotEdit::uiCheckShotEdit(uiParent* p, Well::Data& wd, 
				const char* vellog, bool issonic ) 
    : uiDialog(p,uiDialog::Setup("Checkshot edtitor",
		"Edit checkshot based on integrated velocities",
		mTODOHelpID))
    , d2t_(0)
    , orgcs_(0)
    , cs_(wd.checkShotModel()) 
{
    if ( !cs_ ) 
	mErrRet( "No checkshot provided" )

    orgcs_ = new Well::D2TModel( *cs_ );

    Well::Log* log = wd.logs().getLog( vellog );
    if ( !log ) 
	mErrRet( "Unvalid velocity log provided" )

    const Well::Track& track = wd.track();
    float rdelev = track.dah( 0 ) - track.value( 0 );
    if ( mIsUdf( rdelev ) ) rdelev = 0;

    const Well::Info& info = wd.info();
    float surfelev = mIsUdf( info.surfaceelev ) ? 0 : -info.surfaceelev;

    const float dah = rdelev - surfelev;

    GeoCalculator gc;
    d2t_ = gc.getModelFromVelLog( *log, dah, issonic );
    if ( !d2t_ )
	mErrRet( "can not generate depth/time model" )

    wd.setD2TModel( d2t_ );
    uiWellDahDisplay::Setup dsu; 
    d2tdisplay_ = new uiWellDahDisplay( this, dsu );
    driftdisplay_ = new uiWellDahDisplay( this, dsu );

    uiWellDahDisplay::Data data;
    data.wd_ = &wd;
    data.dispzinft_ = SI().zInFeet();
    data.zistime_ = true;
    d2tdisplay_->setData( data );
    driftdisplay_->setData( data );
    driftdisplay_->attach( rightOf, d2tdisplay_ );

#define mInitWidth 400
#define mInitHeight 600
    driftdisplay_->setPrefWidth( mInitWidth/2 );
    d2tdisplay_->setPrefWidth( mInitWidth/2 );
    driftdisplay_->setPrefHeight( mInitHeight );
    d2tdisplay_->setPrefHeight( mInitHeight );

    draw();
}


void uiCheckShotEdit::draw()
{
    drawDahObj( d2t_, true, true );
    drawDahObj( cs_, false, true );
    drawDrift();
}


void uiCheckShotEdit::drawDahObj( const Well::DahObj* d, bool first, bool left )
{
    uiWellDahDisplay* disp = left ? d2tdisplay_ : driftdisplay_;
    uiWellDahDisplay::DahObjData& dahdata = disp->dahObjData( first );
    dahdata.setData( d );
    dahdata.xrev_ = false;

    disp->reDraw();
}


void uiCheckShotEdit::drawDrift()
{
    const int sz1 = d2t_->size();
    const int sz2 = cs_->size();
    const int maxsz = mMAX(sz1,sz2);
    const Well::D2TModel* longermdl = sz1 > sz2 ? d2t_ : cs_;

    driftcurve_.erase();
    for ( int idx=0; idx<maxsz; idx++ )
    {
	const float dah = longermdl->dah( idx );
	const float d2tval = d2t_->getTime( dah );
	const float csval = cs_->getTime( dah );

	driftcurve_.add( dah, csval - d2tval ); 
    }

    drawDahObj( &driftcurve_, true, false );
}

}
