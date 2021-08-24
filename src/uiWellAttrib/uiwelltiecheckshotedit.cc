/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Oct 2011
________________________________________________________________________

-*/


#include "uiwelltiecheckshotedit.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigraphicsscene.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiwelldlgs.h"
#include "uiwelldahdisplay.h"
#include "uiwelldisplaycontrol.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "od_helpids.h"
#include "sorting.h"


#define mErrRet(msg,act) { uiMSG().error( msg ); act; }

namespace WellTie
{

bool uiCheckShotEdit::DriftCurve::insertAtDah( float dh, float v )
{
    if ( mIsUdf(v) ) return false;
    if ( dah_.isEmpty()|| dh > dah_[dah_.size()-1] )
	{ dah_ += dh; val_ += v; }
    else if ( dh < dah_[0] )
	{ dah_.insert( 0, dh ); val_.insert( 0, v ); }
    else
    {
	const int insertidx = indexOf( dh );
	if ( insertidx>=0 )
	    { dah_.insert( insertidx+1, dh ); val_.insert( insertidx+1, v ); }
    }
    return true;
}


int uiCheckShotEdit::DriftCurve::indexOfCurrentPoint(float dh,float val) const
{
    for ( int idx=0; idx<size(); idx ++ )
    {
	const float nddah = dah( idx );
	if ( mIsEqual(dh,nddah,10) )
	{
	    const float ndval = value( idx );
	    if ( mIsEqual(val,ndval,5) )
		return idx;
	}
    }
    return -1;
}


uiCheckShotEdit::uiCheckShotEdit(uiParent* p, Server& server )
    : uiDialog(p,uiDialog::Setup(tr("Apply Checkshot correction"),
		tr("Edit depth/time model based on checkshot"),
		mODHelpKey(mCheckShotEditHelpID) ).nrstatusflds(1))
    , server_(server)
    , wd_(*server.wd())
    , d2tlineitm_(0)
    , d2t_(wd_.d2TModel())
    , tkzs_(wd_.checkShotModel())
    , orgcs_(0)
    , isedit_(false)
    , movingpointidx_(-1)
{
    if ( !tkzs_ )
	mErrRet( tr("No checkshot provided"), return );

    orgcs_ = new Well::D2TModel( *tkzs_ );
    tkzs_->setName( "CheckShot Curve" );

    newdriftcurve_.setName( "User defined Drift Curve" );

    orgd2t_ = new Well::D2TModel( *d2t_ );
    uiWellDahDisplay::Setup dsu;
    dsu.samexaxisrange_ = true; dsu.drawcurvenames_ = false;
    d2tdisplay_ = new uiWellDahDisplay( this, dsu );
    dsu.symetricalxaxis_ = true;
    driftdisplay_ = new uiWellDahDisplay( this, dsu );
    driftdisplay_->setToolTip( tr("Pick to edit drift curve"));

    uiWellDahDisplay::Data data( &wd_ );
    data.dispzinft_ = SI().zInFeet();
    data.zistime_ = false;
    d2tdisplay_->setData( data );
    driftdisplay_->setData( data );
    driftdisplay_->attach( rightOf, d2tdisplay_ );

    BufferStringSet markernms;
    wd_.markers().getNames( markernms );
    d2tdisplay_->markerDisp().setMarkerNms( markernms, false );
    driftdisplay_->markerDisp().setMarkerNms( markernms, false );

#define mInitWidth 400
#define mInitHeight 600
    driftdisplay_->setPrefWidth( mInitWidth/2 );
    d2tdisplay_->setPrefWidth( mInitWidth/2 );
    driftdisplay_->setPrefHeight( mInitHeight );
    d2tdisplay_->setPrefHeight( mInitHeight );

#define mAddButton(pm,func,tip) \
	toolbar_->addButton( pm, tip, mCB(this,uiCheckShotEdit,func) )
    toolbar_ = new uiToolBar( this, tr("Well Tie Control"), uiToolBar::Right );
    mAddButton( "z2t", editCSPushed, tr("View/Edit Model") );
    toolbar_->addSeparator();
    editbut_ = new uiToolButton( toolbar_, "seedpickmode",tr("Edit mode"),
				mCB(this,uiCheckShotEdit,editCB) );
    toolbar_->addObject( editbut_ );
    editbut_->setToggleButton( true );

    undobut_ = new uiToolButton( toolbar_, "undo", uiStrings::sUndo(),
				mCB(this,uiCheckShotEdit,undoCB) );
    toolbar_->addObject( undobut_ );
    undobut_->setSensitive( false );
    redobut_ = new uiToolButton( toolbar_, "redo", uiStrings::sRedo(),
				mCB(this,uiCheckShotEdit,redoCB) );
    toolbar_->addObject( redobut_ );
    redobut_->setSensitive( false );

    control_ = new uiWellDisplayControl( *d2tdisplay_ );
    control_->addDahDisplay( *driftdisplay_ );
    control_->posChanged.notify( mCB(this,uiCheckShotEdit,setInfoMsg) );
    control_->posChanged.notify( mCB(this,uiCheckShotEdit,mouseMovedCB) );
    control_->mousePressed.notify( mCB(this,uiCheckShotEdit,mousePressedCB));
    control_->mouseReleased.notify( mCB(this,uiCheckShotEdit,mouseReleasedCB));

    viewcorrd2t_ = new uiCheckBox( this, tr("View corrected Depth/Time model"));
    viewcorrd2t_->attach( alignedBelow, d2tdisplay_ );
    viewcorrd2t_->attach( ensureBelow, driftdisplay_ );
    viewcorrd2t_->setChecked( true );

    uiLabeledComboBox* lbl = new uiLabeledComboBox( this,
				    tr("Compute Depth/Time model from: ") );
    driftchoicefld_ = lbl->box();
    lbl->attach( ensureBelow, viewcorrd2t_ );
    driftchoicefld_->addItem( tr("Original drift curve") );
    driftchoicefld_->addItem( tr("User defined drift curve") );

    CallBack applycb( mCB(this,uiCheckShotEdit,applyCB) );
    driftchoicefld_->selectionChanged.notify( applycb );
    viewcorrd2t_->activated.notify( applycb );
    d2tdisplay_->reSize.notify( mCB(this,uiCheckShotEdit,reSizeCB) );

    drawDrift();
    applyCB(0);
}


uiCheckShotEdit::~uiCheckShotEdit()
{
    delete control_;
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
	statusBar()->message( mToUiStringTodo(inf.buf()) );
    }
}


void uiCheckShotEdit::mousePressedCB( CallBacker* )
{
    const float dah = control_->dah();
    const float val = control_->xPos();
    movingpointidx_ = isedit_ ? newdriftcurve_.indexOfCurrentPoint(dah,val) :-1;
}


void uiCheckShotEdit::mouseMovedCB( CallBacker* )
{
    if ( movingpointidx_ >= 0 && isedit_ )
	movePt();
}


void uiCheckShotEdit::mouseReleasedCB( CallBacker* )
{
    if ( movingpointidx_ < 0 && isedit_ )
	doInsertRemovePt();

    movingpointidx_ = -1;
}


void uiCheckShotEdit::reSizeCB( CallBacker* )
{
    drawPoints();
}


void uiCheckShotEdit::movePt()
{
    if ( movingpointidx_ < 0 )
	return;

    const float dah = control_->dah();

    if ( movingpointidx_ > 1 )
    {
	const float prevdah = newdriftcurve_.dah( movingpointidx_-1 );
	if ( dah < prevdah )
	    return;
    }
    if ( newdriftcurve_.size() >  movingpointidx_+1 )
    {
	const float nextdah = newdriftcurve_.dah( movingpointidx_+1 );
	if ( dah > nextdah  )
	    return;
    }

    newdriftcurve_.remove( movingpointidx_ );
    doInsertRemovePt();
}


void uiCheckShotEdit::doInsertRemovePt()
{
    if ( isedit_ )
    {
	const float dah = control_->dah();
	const float val = control_->xPos();

	const bool isadd = !control_->isCtrlPressed();
	if ( isadd )
	{
	    if ( newdriftcurve_.isEmpty() && !driftcurve_.isEmpty() )
		newdriftcurve_.add( driftcurve_.dah(0), driftcurve_.value(0) );

	    newdriftcurve_.insertAtDah( dah, val );
	}
	undo_.addEvent( new DahObjUndoEvent(dah,val,newdriftcurve_,isadd) );
    }
    undobut_->setSensitive( undo_.canUnDo() );
    redobut_->setSensitive( undo_.canReDo() );
    driftdisplay_->setToolTip(uiString::emptyString());

    applyCB(0);
}


void uiCheckShotEdit::editCSPushed( CallBacker* )
{
    uiD2TModelDlg csmdlg( this, wd_, true );
    if ( csmdlg.go() )
    {
	tkzs_ = wd_.checkShotModel();
	draw();
    }
}


void uiCheckShotEdit::editCB( CallBacker* )
{
    isedit_ = editbut_->isOn();
}


void uiCheckShotEdit::undoCB( CallBacker* )
{
    undo_.unDo(1,false);
    undobut_->setSensitive( undo_.canUnDo() );
    redobut_->setSensitive( undo_.canReDo() );
    draw();
}


void uiCheckShotEdit::redoCB( CallBacker* )
{
    undo_.reDo(1,false);
    undobut_->setSensitive( undo_.canUnDo() );
    redobut_->setSensitive( undo_.canReDo() );
    draw();
}


void uiCheckShotEdit::draw()
{
    drawDahObj( orgd2t_, true, true );
    drawDahObj( tkzs_, false, true );
    drawDrift();
}


void uiCheckShotEdit::drawDahObj( const Well::DahObj* d, bool first, bool left )
{
    uiWellDahDisplay* disp = left ? d2tdisplay_ : driftdisplay_;
    uiWellDahDisplay::DahObjData& dahdata = disp->dahObjData( first );
    dahdata.col_ = d == &newdriftcurve_ ? Color::DgbColor()
					: Color::stdDrawColor(first ? 0 : 1);
    float zfac = 1.f;
    if ( SI().depthsInFeet() ) zfac = mToFeetFactorF;
    float startpos = -SI().seismicReferenceDatum();
    const float stoppos = (orgcs_->dahRange().stop > orgd2t_->dahRange().stop)
	? orgcs_->dahRange().stop : orgd2t_->dahRange().stop;
    Interval<float> zrg( startpos*zfac, stoppos*zfac );
    disp->setZRange(zrg);
    dahdata.drawaspoints_ = d == tkzs_ || d == &newdriftcurve_;
    dahdata.xrev_ = false;
    dahdata.setData( d );

    disp->reDraw();
}


void uiCheckShotEdit::drawDrift()
{
    const int sz1 = orgd2t_->size();
    const int sz2 = tkzs_->size();
    int maxsz = mMAX(sz1,sz2);
    const Well::D2TModel* longermdl = sz1 > sz2 ? orgd2t_ : tkzs_;

    driftcurve_.setEmpty();
    for ( int idx=0; idx<maxsz; idx++ )
    {
	const float dah = longermdl->dah( idx );
	const float d2tval = orgd2t_->getTime( dah, wd_.track() );
	const float csval = tkzs_->getTime( dah, wd_.track() );
	const float drift = SI().zDomain().userFactor()*( csval - d2tval );
	driftcurve_.add( dah, drift );
    }
    drawDahObj( &driftcurve_, true, false );
    drawDahObj( &newdriftcurve_, false, false );

    for ( int idx=0; idx<sz2; idx++ )
    {
	const float dah = tkzs_->dah( idx );
	const float d2tval = orgd2t_->getTime( dah, wd_.track() );
	const float csval = tkzs_->value( idx );
	const float drift = SI().zDomain().userFactor()*( csval - d2tval );
	uiWellDahDisplay::PickData pd( dah, Color::stdDrawColor( 0 ) );
	pd.val_ = drift;
	driftcurve_.insertAtDah( dah, drift );
	driftdisplay_->zPicks() += pd;
    }

    float startpos = -SI().seismicReferenceDatum();
    const float stoppos = (orgcs_->dahRange().stop > orgd2t_->dahRange().stop)
	? orgcs_->dahRange().stop : orgd2t_->dahRange().stop;
    float zfac = 1.f;
    if ( SI().depthsInFeet() ) zfac = mToFeetFactorF;

    Interval<float> zrg( startpos*zfac, stoppos*zfac );
    driftdisplay_->setZRange(zrg);
}


void uiCheckShotEdit::drawPoints()
{
    uiGraphicsScene& scene = d2tdisplay_->scene();
    scene.removeItem( d2tlineitm_ );
    delete d2tlineitm_; d2tlineitm_=0;
    float zfac = 1.f;
    if ( SI().depthsInFeet() ) zfac = mToFeetFactorF;
    if ( viewcorrd2t_->isChecked() )
    {
	TypeSet<uiPoint> pts;
	uiWellDahDisplay::DahObjData& ld = d2tdisplay_->dahObjData(true);
	for ( int idx=0; idx<d2t_->size(); idx++ )
	{
	    const float val = d2t_->value( idx );
	    const float dah = (float) wd_.track().getPos(
						    (d2t_->dah( idx ))*zfac ).z;
	    pts += uiPoint( ld.xax_.getPix(val), ld.yax_.getPix(dah) );
	}

	if ( pts.isEmpty() ) return;

	d2tlineitm_ = scene.addItem( new uiPolyLineItem(pts) );
	OD::LineStyle ls( OD::LineStyle::Solid, 2, Color::DgbColor() );
	d2tlineitm_->setPenStyle( ls );
    }
}


void uiCheckShotEdit::applyCB( CallBacker* )
{
    const bool isorgdrift = driftchoicefld_->currentItem() == 0;
    editbut_->setSensitive( !isorgdrift );
    const DriftCurve& driftcurve = isorgdrift ? driftcurve_ : newdriftcurve_;
    Well::D2TModel tmpcs;
    *d2t_ = *orgd2t_;

    for ( int idx=0; idx<driftcurve.size(); idx++ )
    {
	const float dah = driftcurve.dah( idx );
	const float drift = driftcurve.value( idx );
	const float d2tval = orgd2t_->getTime( dah, wd_.track() );
	const float csval = drift / SI().zDomain().userFactor() + d2tval;
	tmpcs.add( dah, csval );
    }

    CheckShotCorr::calibrate( tmpcs, *d2t_ );
    drawPoints();
    draw();
}


bool uiCheckShotEdit::acceptOK( CallBacker* )
{
    if ( !d2t_ || d2t_->size() < 2)
	mErrRet(tr("Depth/time model is too small and won't be saved"),
		return false)

    server_.d2TModelMgr().setAsCurrent( new Well::D2TModel( *d2t_ ) );
	server_.updateExtractionRange();

    return true;
}


bool uiCheckShotEdit::rejectOK( CallBacker* )
{
    server_.d2TModelMgr().cancel();
    return true;
}



DahObjUndoEvent::DahObjUndoEvent( float dah, float val,
				Well::DahObj& dahobj, bool isadd )
    : dah_(dah)
    , val_(val)
    , dahobj_(dahobj)
    , isadd_(isadd)
{}


const char* DahObjUndoEvent::getStandardDesc() const
{ return "Add/Remove point in drift curve"; }


bool DahObjUndoEvent::unDo()
{
    if ( isadd_ )
    {
	const int dahidx = dahobj_.indexOf( dah_ );
	if ( dahidx >= 0 && dahidx < dahobj_.size() )
	    dahobj_.remove( dahidx );
    }
    else
    {
	dahobj_.insertAtDah( dah_, val_ );
    }
    return true;
}


bool DahObjUndoEvent::reDo()
{
    if ( isadd_ )
    {
	dahobj_.insertAtDah( dah_, val_ );
    }
    else
    {
	const int dahidx = dahobj_.indexOf( dah_ );
	if ( dahidx >= 0 && dahidx < dahobj_.size() )
	    dahobj_.remove( dahidx );
    }
    return true;
}

} // namespace WellTie
