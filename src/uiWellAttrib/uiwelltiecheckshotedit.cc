/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Oct 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiecheckshotedit.cc,v 1.3 2011-10-28 09:58:55 cvsbruno Exp $";

#include "uiwelltiecheckshotedit.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiwelldahdisplay.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"


#define mErrRet(msg) { uiMSG().error( msg ); return; }

static const char* styles[] = { "Curve", "Points", "Both", 0 };

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
    , dodrawpoints_(false)			      
    , dodrawcurves_(true)
    , dointerpolatecs_(true)		     	
{
    if ( !cs_ ) 
	mErrRet( "No checkshot provided" )

    orgcs_ = new Well::D2TModel( *cs_ );
    cs_->setName( "CheckShot" );

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

    d2t_->setName( "Depth/Time Model");
    wd.setD2TModel( d2t_ );
    uiWellDahDisplay::Setup dsu; 
    dsu.sameaxisrange_ = true; dsu.drawcurvenames_ = false;
    d2tdisplay_ = new uiWellDahDisplay( this, dsu );
    driftdisplay_ = new uiWellDahDisplay( this, dsu );

    uiWellDahDisplay::Data data;
    data.wd_ = &wd;
    data.dispzinft_ = SI().zInFeet();
    data.zistime_ = true;
    data.zrg_ = SI().zRange(false);
    data.zrg_.scale( SI().zFactor() );
    d2tdisplay_->setData( data );
    driftdisplay_->setData( data );
    driftdisplay_->attach( rightOf, d2tdisplay_ );

#define mInitWidth 400
#define mInitHeight 600
    driftdisplay_->setPrefWidth( mInitWidth/2 );
    d2tdisplay_->setPrefWidth( mInitWidth/2 );
    driftdisplay_->setPrefHeight( mInitHeight );
    d2tdisplay_->setPrefHeight( mInitHeight );

    CallBack parcb( mCB(this,uiCheckShotEdit,parChg) );
    interpolbox_ = new uiCheckBox( this, "Interpolate CheckShot");
    interpolbox_->setChecked( dointerpolatecs_ );
    interpolbox_->activated.notify( parcb );
    interpolbox_->attach( ensureBelow, d2tdisplay_ );
    interpolbox_->attach( alignedBelow, driftdisplay_ );

    uiLabeledComboBox* lbl = new uiLabeledComboBox(this, "Select draw style: ");
    stylefld_ = lbl->box();
    lbl->attach( hCentered );
    stylefld_->selectionChanged.notify( parcb );
    stylefld_->addItems( styles ); 
    lbl->attach( ensureBelow, interpolbox_ );

    draw();
}


void uiCheckShotEdit::parChg( CallBacker* )
{
    dointerpolatecs_ = interpolbox_->isChecked();
    const int setp = stylefld_->currentItem();
    dodrawpoints_ = setp == 1 || setp == 2;
    dodrawcurves_ = setp == 0 || setp == 2;
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
    dahdata.col_ = Color::stdDrawColor( first ? 0 : 1 );
    dahdata.setData( d );
    dahdata.xrev_ = false;
    dahdata.drawascurve_ = dodrawcurves_; 
    dahdata.drawaspoints_ = dodrawpoints_; 

    disp->reDraw();
}


void uiCheckShotEdit::drawDrift()
{
    const int sz1 = d2t_->size();
    const int sz2 = cs_->size();
    int maxsz = mMAX(sz1,sz2);
    const Well::D2TModel* longermdl = sz1 > sz2 ? d2t_ : cs_;

    if ( !dointerpolatecs_ )
    {
	longermdl = cs_;
	maxsz = sz2;
    }

    driftcurve_.erase();
    for ( int idx=0; idx<maxsz; idx++ )
    {
	const float dah = longermdl->dah( idx );
	const float d2tval = d2t_->getTime( dah );
	const float csval = cs_->getTime( dah );
	driftcurve_.add( dah, d2tval - csval ); 
    }

    drawDahObj( &driftcurve_, true, false );
}

}

