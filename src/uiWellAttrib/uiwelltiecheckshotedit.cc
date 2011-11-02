/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Oct 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiecheckshotedit.cc,v 1.5 2011-11-02 15:27:52 cvsbruno Exp $";

#include "uiwelltiecheckshotedit.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigraphicsscene.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiwelldlgs.h"
#include "uiwelldahdisplay.h"
#include "uiwelldisplaycontrol.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiecshot.h"
#include "welltiegeocalculator.h"


#define mErrRet(msg,act) { uiMSG().error( msg ); act; }

static const char* styles[] = { "Curve", "Points", "Both", 0 };

namespace WellTie
{
uiCheckShotEdit::uiCheckShotEdit(uiParent* p, Well::Data& wd, 
				const char* vellog, bool issonic ) 
    : uiDialog(p,uiDialog::Setup("Checkshot edtitor",
		"Edit checkshot based on integrated velocities",
		mTODOHelpID).nrstatusflds(1))
    , d2t_(0)
    , orgd2t_(0)
    , orgcs_(0)
    , wd_(wd)      
    , cs_(wd.checkShotModel())
    , dodrawpoints_(false)			      
    , dodrawcurves_(true)
    , isedit_(false)			 
{
    if ( !cs_ ) 
	mErrRet( "No checkshot provided", return )

    orgcs_ = new Well::D2TModel( *cs_ );
    cs_->setName( "CheckShot Curve" );

    Well::Log* log = wd.logs().getLog( vellog );
    if ( !log ) 
	mErrRet( "Unvalid velocity log provided", return )

    const Well::Track& track = wd.track();
    float rdelev = track.dah( 0 ) - track.value( 0 );
    if ( mIsUdf( rdelev ) ) rdelev = 0;

    const Well::Info& info = wd.info();
    float surfelev = mIsUdf( info.surfaceelev ) ? 0 : -info.surfaceelev;

    const float dah = rdelev - surfelev;

    GeoCalculator gc;
    d2t_ = gc.getModelFromVelLog( *log, dah, issonic );
    if ( !d2t_ )
	mErrRet( "can not generate depth/time model" , return)

    d2t_->setName( "Original Depth/Time Model");
    orgd2t_ = new Well::D2TModel( *d2t_ );
    wd.setD2TModel( d2t_ );
    uiWellDahDisplay::Setup dsu; 
    dsu.samexaxisrange_ = true; dsu.drawcurvenames_ = false;
    d2tdisplay_ = new uiWellDahDisplay( this, dsu );
    dsu.symetricalxaxis_ = true;
    driftdisplay_ = new uiWellDahDisplay( this, dsu );

    uiWellDahDisplay::Data data;
    data.wd_ = &wd;
    data.dispzinft_ = SI().zInFeet();
    data.zistime_ = false;
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

#define mAddButton(pm,func,tip) \
	toolbar_->addButton( pm, tip, mCB(this,uiCheckShotEdit,func) )
    toolbar_ = new uiToolBar( this, "Well Tie Control", uiToolBar::Right );
    mAddButton( "z2t.png", editCSPushed, "View/Edit Model" );
    editbut_ = new uiToolButton( toolbar_, "seedpickmode.png","Edit mode",
				mCB(this,uiCheckShotEdit,editCB) );
    toolbar_->addButton( editbut_ );
    editbut_->setToggleButton( isedit_ );

    draw();

    control_ = new uiWellDisplayControl( *d2tdisplay_ );
    control_->addDahDisplay( *driftdisplay_ );
    control_->posChanged.notify( mCB(this,uiCheckShotEdit,setInfoMsg) );
    control_->mouseReleased.notify( mCB(this,uiCheckShotEdit,mouseReleasedCB));
}


uiCheckShotEdit::~uiCheckShotEdit()
{
    delete orgd2t_;
    delete orgcs_;
}


void uiCheckShotEdit::setInfoMsg( CallBacker* cb )
{
    BufferString infomsg;
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh )
    {
	mCBCapsuleUnpack(BufferString,inf,cb);
	statusBar()->message( inf.buf() );
    }
}


void uiCheckShotEdit::mouseReleasedCB( CallBacker* )
{
    const uiWellDahDisplay* disp = control_->selDahDisplay();
    if ( disp == d2tdisplay_ && isedit_ ) 
    {
	const float xval = control_->xPos(); 
	const float yval = control_->yPos();
	cs_->insertAtDah( xval, yval );
    }
    draw();
}


void uiCheckShotEdit::editCSPushed( CallBacker* )
{
    uiD2TModelDlg csmdlg( this, wd_, true );
    if ( csmdlg.go() )
    {
	cs_ = wd_.checkShotModel();
	draw();
    }
}


void uiCheckShotEdit::parChg( CallBacker* )
{
    draw();
}


void uiCheckShotEdit::editCB( CallBacker* )
{
    isedit_ = !editbut_->isOn();
    editbut_->setToggleButton( isedit_ );
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
    dahdata.drawascurve_ = dodrawcurves_ && d != &csdriftcurve_; 
    dahdata.drawaspoints_ = ( d == cs_ ) || ( d == &csdriftcurve_ );

    disp->reDraw();
}


void uiCheckShotEdit::drawDrift()
{
    const int sz1 = d2t_->size();
    const int sz2 = cs_->size();
    int maxsz = mMAX(sz1,sz2);
    const Well::D2TModel* longermdl = sz1 > sz2 ? d2t_ : cs_;

    driftcurve_.erase();
    for ( int idx=0; idx<maxsz; idx++ )
    {
	const float dah = longermdl->dah( idx );
	const float d2tval = d2t_->getTime( dah );
	const float csval = cs_->getTime( dah );
	const float drift = SI().zFactor()*( csval - d2tval );
	driftcurve_.add( dah, drift ); 
    }
    for ( int idx=0; idx<sz2; idx++ )
    {
	const float dah = cs_->dah( idx );
	const float d2tval = d2t_->getTime( dah );
	const float csval = cs_->value( idx );
	const float drift = SI().zFactor()*( csval - d2tval );
	csdriftcurve_.add( dah, drift );
    }

    drawDahObj( &driftcurve_, true, false );
    drawDahObj( &csdriftcurve_, false, false );
}


bool uiCheckShotEdit::acceptOK( CallBacker* )
{
    if ( !d2t_ || d2t_->size() < 2)
	mErrRet("Depth/time model is too small and won't be saved",return false)

    wd_.setD2TModel( d2t_ );
    return true; 
}


}

