/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/

#include "visrandomtrackdragger.h"

#include "dragcontroller.h"
#include "visdragger.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistransform.h"
#include "survinfo.h"
#include "mouseevent.h"
#include "linerectangleclipper.h"

#include <osg/Switch>
#include <osg/PolygonOffset>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>
#include <osg/Material>
#include <osgGeo/TabPlaneDragger>

mCreateFactoryEntry( visBase::RandomTrackDragger );

namespace visBase
{


class PlaneDragCBHandler : public osgManipulator::DraggerCallback
{
    public:
				PlaneDragCBHandler(RandomTrackDragger&);

    using			osgManipulator::DraggerCallback::receive;
    bool			receive(const osgManipulator::MotionCommand&)
								      override;

    enum DragMode		{ Scale1D, Trans1D, Trans2D, Rotate, Ignore };

    osgGeo::TabPlaneDragger&	osgDragger()	{ return *planedragger_; }

    bool			isMoving() const	{ return ismoving_; }

    bool			setCorners(const Coord3& worldtopleft,
					   const Coord3& worldbotright);

    void			showDraggerBorder(bool yn);
    void			showDraggerTabs(bool yn);

    void			setTransModKeyMask(bool trans1d,int mask,
						   int groupidx);
    int				getTransModKeyMask(bool trans1d,
						   int groupidx) const;

protected:

    void			constrain(int planeidx,DragMode);

    bool			getCorners(Coord3& worldtopleft,
					   Coord3& worldbotright) const;

    void			extendStickyKnots(Coord& worldleftpos,
						  Coord& worldrightpos) const;

    void			slowDownTrans1D(Coord3& newtopleft,
						Coord3& newbotright,
						double* maxdragdistptr=0) const;

    void			initDragControl();
    void			applyDragControl(Coord3& newtopleft,
						 Coord3& newbotright);

    RandomTrackDragger&		rtdragger_;

    bool			ismoving_;
    double			extension_;

    osg::Vec2			initialmousepos_;
    osg::Matrix			initialosgmatrix_;
    Coord3			initialtopleft_;
    Coord3			initialbotright_;

    bool			isleftsticky_;
    bool			isrightsticky_;

    osg::Vec3			localdir_;
    osg::Vec2			pickedpos_;

    DragController		dragcontroller_;
    double			maxdragdist_;

    osg::ref_ptr<osgGeo::TabPlaneDragger> planedragger_;
};


PlaneDragCBHandler::PlaneDragCBHandler( RandomTrackDragger& rtd )
    : rtdragger_( rtd )
    , ismoving_( false )
    , localdir_( 0.0, 0.0, 0.0 )
    , extension_( mMAX(SI().reasonableRange(true).width(),
		       SI().reasonableRange(false).width()) )
{
    planedragger_ = new osgGeo::TabPlaneDragger( 12.0 );
    planedragger_->setIntersectionMask( cDraggerIntersecTraversalMask() );
    planedragger_->setActivationMouseButtonMask(
				osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON |
				osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON );
    planedragger_->setupDefaultGeometry();
    planedragger_->setHandleEvents( rtd.isHandlingEvents() );

    for ( int idx=planedragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = planedragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	if ( s2dd )
	{
	    planedragger_->removeChild( dragger );
	    planedragger_->removeDragger( dragger );
	}
    }

    for ( int idx=0; idx<rtdragger_.dragcontrols_.size(); idx++ )
    {
	setTransModKeyMask( rtdragger_.dragcontrols_[idx]->trans1d_,
			    rtdragger_.dragcontrols_[idx]->modkeymask_,
			    rtdragger_.dragcontrols_[idx]->groupidx_ );
    }

    showDraggerBorder( true );
    showDraggerTabs( true );
}


bool PlaneDragCBHandler::receive( const osgManipulator::MotionCommand& cmd )
{
    const int planeidx = rtdragger_.planedraghandlers_.indexOf( this );
    if ( planeidx < 0 )
	return false;

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, scale1d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInLineCommand*,
		     trans1d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     trans2d, &cmd );

    DragMode dragmode = scale1d ? Scale1D :
			trans1d ? Trans1D :
			trans2d ? Trans2D : Ignore;

    if ( trans1d && planedragger_->getCurMouseButtonModKeyIdx()==1 )
	dragmode = Rotate;

    const TabletInfo* ti = TabletInfo::currentState();

    if ( ti && ti->maxPostPressDist()<5 )
	dragmode = Ignore;

    if ( cmd.getStage() == osgManipulator::MotionCommand::START )
    {
	initialosgmatrix_ = planedragger_->getMatrix();

	getCorners( initialtopleft_, initialbotright_ );

	initialmousepos_ = planedragger_->getNormalizedPosOnScreen();
	pickedpos_ = planedragger_->getNormalizedPosOnPlane();

	isleftsticky_ = rtdragger_.doesKnotStickToBorder( planeidx );
	isrightsticky_ = rtdragger_.doesKnotStickToBorder( planeidx+1 );

	rtdragger_.showRotationAxis( dragmode==Rotate, planeidx,
				     Conv::to<Coord>(pickedpos_) );
	initDragControl();
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	if ( dragmode != Ignore )
	{
	    ismoving_ = true;
	    rtdragger_.postponePanelUpdate( true );
	    constrain( planeidx, dragmode );
	    rtdragger_.postponePanelUpdate( false );
	    rtdragger_.motion.trigger( -planeidx-1 );
	}
	else
	    planedragger_->setMatrix( initialosgmatrix_ );
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	rtdragger_.showRotationAxis( false );

	if ( ismoving_ )
	{
	    ismoving_ = false;

	    if ( dragmode != Ignore )
	    {
		rtdragger_.updatePlaneDraggers();
		rtdragger_.movefinished.trigger();
	    }
	    else
		planedragger_->setMatrix( initialosgmatrix_ );
	}
    }

    return true;
}


void PlaneDragCBHandler::extendStickyKnots( Coord& worldleftpos,
					    Coord& worldrightpos ) const
{
    Coord hordir = worldrightpos - worldleftpos;
    hordir.normalize();

    if ( isleftsticky_ )
	worldleftpos -= hordir * extension_;
    if ( isrightsticky_ )
	worldrightpos += hordir * extension_;
}


void PlaneDragCBHandler::constrain( int planeidx, DragMode dragmode )
{
    LineRectangleClipper<double> clipper( rtdragger_.horborder_ );

    Coord3 newtopleft, newbotright;
    getCorners( newtopleft, newbotright );

    const Coord3 initialcenter = 0.5 * (initialtopleft_+initialbotright_);
    const Coord initialhordif = initialbotright_ - initialtopleft_;

    if ( dragmode == Trans1D )
    {
	applyDragControl( newtopleft, newbotright );
	slowDownTrans1D( newtopleft, newbotright, &maxdragdist_ );
    }

    if ( dragmode == Rotate )
    {
	const osg::Vec2& mousepos = planedragger_->getNormalizedPosOnScreen();
	float angle = M_PI * (mousepos-initialmousepos_)[0];
	if ( !SI().isRightHandSystem() )
	    angle = -angle;

	const Coord dir( cos(angle), sin(angle) );
	Coord newhordif( initialhordif.dot(dir),
			 initialhordif.dot(Coord(-dir.y,dir.x)) );

	const Coord pivot = initialtopleft_ * (1.0-pickedpos_[0]) +
			    initialbotright_ * pickedpos_[0];

	newtopleft.coord()  = pivot - newhordif * pickedpos_[0];
	newbotright.coord() = pivot + newhordif * (1.0-pickedpos_[0]);
    }

    if ( dragmode==Rotate || dragmode==Trans1D )
	extendStickyKnots( newtopleft, newbotright );

    clipper.setLine( newtopleft, newbotright );
    newtopleft.coord() = clipper.getStart();
    newbotright.coord() = clipper.getStop();

    if ( dragmode==Trans2D )
    {
	if ( !clipper.isIntersecting() )
	{
	    const bool movesleft = newtopleft.sqDistTo(initialcenter) >
				   newbotright.sqDistTo(initialcenter);
	    if ( movesleft )
	    {
		clipper.setLine( newtopleft, initialcenter );
		newtopleft.coord() = clipper.getStart();
	    }
	    else
	    {
		clipper.setLine( initialcenter, newbotright );
		newbotright.coord() = clipper.getStop();
	    }
	}

	if ( clipper.isStartChanged() )
	    newbotright.coord() = newtopleft.coord() + initialhordif;
	else if ( clipper.isStopChanged() )
	    newtopleft.coord() = newbotright.coord() - initialhordif;
    }

    Interval<float> zrg( (float) newtopleft.z, (float) newbotright.z );
    Interval<float>& zborder = rtdragger_.zborder_;

    if ( zrg.start < zborder.start )	zrg.start = zborder.start;
    if ( zrg.start > zborder.stop )	zrg.start = zborder.stop;
    if ( zrg.stop < zborder.start )	zrg.stop = zborder.start;
    if ( zrg.stop > zborder.stop )	zrg.stop = zborder.stop;

    if ( dragmode == Trans2D )
    {
	const float zlen = fabs( newbotright.z - newtopleft.z );

	if ( zrg.start==zborder.start || zrg.start==zborder.stop )
	{
	    const int dir = zrg.start<initialcenter.z ? 1 : -1;
	    zrg.stop = zrg.start + dir*zlen;
	}
	else if ( zrg.stop==zborder.start || zrg.stop==zborder.stop )
	{
	    const int dir = zrg.stop<initialcenter.z ? 1 : -1;
	    zrg.start = zrg.stop + dir*zlen;
	}
    }

    newtopleft.coord()	= rtdragger_.horborder_.moveInside( newtopleft );
    newbotright.coord() = rtdragger_.horborder_.moveInside( newbotright );

    rtdragger_.doSetKnot( planeidx, newtopleft );
    rtdragger_.doSetKnot( planeidx+1, newbotright );
    rtdragger_.setDepthRange( zrg );
}


void PlaneDragCBHandler::slowDownTrans1D( Coord3& newtopleft,
					  Coord3& newbotright,
					  double* maxdragdistptr ) const
{
    int dragdepth = 0;

    Geom::Rectangle<double> horborder = rtdragger_.horborder_;

    Coord dragdir = newtopleft - initialtopleft_;
    double dragdist = dragdir.abs();
    if ( !dragdist )
	return;

    dragdir /= dragdist;

    double neardist = mUdf(double);
    double fardist = mUdf(double);

    double outerdist = dragdist + extension_;
    double innerdist = 0.0;

    for ( int isfar=1; isfar>=0; isfar-- )
    {
	LineRectangleClipper<double> clipper( horborder );

	// Use binary search to derive near and far distance to horborder
	for ( int count=0; count<50; count++ )
	{
	    const double avgdist = 0.5 * (innerdist+outerdist);
	    Coord avgleftpos = initialtopleft_.coord() + dragdir*avgdist;
	    Coord avgrightpos = initialbotright_.coord() + dragdir*avgdist;
	    extendStickyKnots( avgleftpos, avgrightpos );
	    clipper.setLine( avgleftpos, avgrightpos );

	    if ( clipper.isIntersecting() )
	    {
		if ( innerdist == avgdist )
		    break;

		innerdist = avgdist;
	    }
	    else
		outerdist = avgdist;
	}

	if ( isfar )
	{
	    fardist = innerdist;
	    if ( fardist >= dragdist+extension_ )   // bounding edge not found
		return;

	    // flip border rectangle around bounding edge to derive neardist
	    if ( fabs(dragdir.x) > fabs(dragdir.y) )
	    {
		if ( dragdir.x > 0 )
		{
		    horborder.setLeft( horborder.right() );
		    horborder.setRight( mUdf(double) );
		}
		else
		{
		    horborder.setRight( horborder.left() );
		    horborder.setLeft( -mUdf(double) );
		}

		dragdepth = SI().inlRange(true).width();
	    }
	    else
	    {
		if ( dragdir.y > 0 )
		{
		    horborder.setTop( horborder.bottom() );
		    horborder.setBottom( mUdf(double) );
		}
		else
		{
		    horborder.setBottom( horborder.top() );
		    horborder.setTop( -mUdf(double) );
		}

		dragdepth = SI().crlRange(true).width();
	    }

	    outerdist = 0.0;
	}
	else
	    neardist = innerdist;
    }

    double maxdragdist = fardist;

    if ( dragdepth>0 )
    {
	const double speedfactor = 0.5;			// emperical
	const double curb = fardist - neardist;
	const double brakepoint = speedfactor * dragdepth;
	const double speed = curb<brakepoint ? curb/brakepoint : 1.0;

	if ( speed > 0.0 )
	    maxdragdist = neardist + curb/speed;

	if ( dragdist >= maxdragdist )
	    dragdist = fardist;
	else if ( dragdist > neardist )
	    dragdist = neardist + speed*(dragdist-neardist);
    }

    newtopleft.coord() = initialtopleft_.coord() + dragdir*dragdist;
    newbotright.coord() = initialbotright_.coord() + dragdir*dragdist;

    if ( maxdragdistptr )
	*maxdragdistptr = maxdragdist;
}


void PlaneDragCBHandler::initDragControl()
{
    const Coord pos = Conv::to<Coord>( planedragger_->getPositionOnScreen() );
    const float scalefactor = mMIN( SI().inlStep(), SI().crlStep() );
    const Coord3 diagonal = initialtopleft_ - initialbotright_;
    const Coord3 dragdir = diagonal.cross( Coord3(0.0,0.0,1.0) );
    dragcontroller_.init( pos, scalefactor, dragdir );
    maxdragdist_ = mUdf(double);

    const bool frontalview =
		    planedragger_->getPlaneNormalAngleToCamera() < M_PI/18.0;

    Coord screendragprojvec = Conv::to<Coord>( frontalview ?
			    planedragger_->getUpwardPlaneAxisProjOnScreen() :
			    planedragger_->getPlaneNormalProjOnScreen() );

    if ( screendragprojvec.sqAbs() )
    {
	screendragprojvec /= screendragprojvec.sqAbs();

	Coord dragdepthvec = dragdir.normalize().coord();
	dragdepthvec[0] *= SI().inlRange(false).width();
	dragdepthvec[1] *= SI().crlRange(false).width();

	screendragprojvec *= dragdepthvec.abs() / scalefactor;
	dragcontroller_.dragInScreenSpace( frontalview, screendragprojvec );
    }
}


void PlaneDragCBHandler::applyDragControl( Coord3& newtopleft,
					   Coord3& newbotright )
{
    Coord3 dragvec = newtopleft - initialtopleft_;
    const Coord pos = Conv::to<Coord>( planedragger_->getPositionOnScreen() );
    dragcontroller_.transform( dragvec, pos, maxdragdist_ );
    newtopleft = initialtopleft_ + dragvec;
    newbotright = initialbotright_ + dragvec;
}


bool PlaneDragCBHandler::setCorners( const Coord3& worldtopleft,
				     const Coord3& worldbotright )
{
    osg::Vec3 localtopleft, localbotright;
    mVisTrans::transform( rtdragger_.displaytrans_,
			  worldtopleft, localtopleft );
    mVisTrans::transform( rtdragger_.displaytrans_,
			  worldbotright, localbotright );

    osg::Vec3 localdir = osg::Vec3( localbotright[0]-localtopleft[0],
				    localbotright[1]-localtopleft[1], 0.0 );
    const double dirlen = localdir.length();
    if ( dirlen <= 0.0 )
	return false;

    localdir_ = localdir / dirlen;
    const double zlen = fabs( localbotright[2]-localtopleft[2] );

    osg::Matrix mat = osg::Matrix::scale( osg::Vec3(dirlen,1.0,zlen) );
    mat *= osg::Matrix::rotate( osg::Vec3(1.0,0.0,0.0), localdir_ );
    mat *= osg::Matrix::translate( (localtopleft+localbotright)*0.5 );
    planedragger_->setMatrix( mat );

    return true;
}


bool PlaneDragCBHandler::getCorners( Coord3& worldtopleft,
				     Coord3& worldbotright ) const
{
    if ( localdir_ == osg::Vec3(0.0,0.0,0.0) )
	return false;

    osg::Matrix mat = planedragger_->getMatrix();
    const osg::Vec3 trans = mat.getTrans();
    mat *= osg::Matrix::rotate( localdir_, osg::Vec3(1.0,0.0,0.0) );
    const osg::Vec3 scale = mat.getScale();

    const osg::Vec3 diagonal( scale[0]*localdir_[0],
			      scale[0]*localdir_[1], scale[2] );
    const osg::Vec3 localtopleft = trans - diagonal * 0.5;
    const osg::Vec3 localbotright = localtopleft + diagonal;

    mVisTrans::transformBack( rtdragger_.displaytrans_,
			      localtopleft, worldtopleft );
    mVisTrans::transformBack( rtdragger_.displaytrans_,
			      localbotright, worldbotright );
    return true;
}


void PlaneDragCBHandler::showDraggerBorder( bool yn )
{
    const float borderopacity = yn ? 1.0 : 0.0;
    for ( int idx=planedragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 planedragger_->getDragger(idx) );
	if ( tpd )
	{
	    const osg::Vec4 col( 0.5, 0.5, 0.5, borderopacity );
	    if ( col != tpd->getTranslate2DDragger()->getColor() )
		tpd->getTranslate2DDragger()->setColor( col );

	    const osg::Vec4 pickcol( 1.0, 1.0, 1.0, borderopacity );
	    if ( pickcol != tpd->getTranslate2DDragger()->getPickColor() )
		tpd->getTranslate2DDragger()->setPickColor( pickcol );
	}
    }
}


void PlaneDragCBHandler::showDraggerTabs( bool yn )
{
    const float tabopacity = yn ? 1.0 : 0.0;
    for ( int idx=planedragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = planedragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	if ( s1dd )
	{
	    s1dd->setColor( osg::Vec4(0.0,0.7,0.0,tabopacity) );
	    s1dd->setPickColor( osg::Vec4(0.0,1.0,0.0,tabopacity) );
	}
    }
}


void PlaneDragCBHandler::setTransModKeyMask( bool trans1d, int mask,
					     int groupidx )
{
    if ( trans1d )
	planedragger_->set1DTranslateModKeyMask( mask, groupidx );
    else
	planedragger_->set2DTranslateModKeyMask( mask, groupidx );
}


int PlaneDragCBHandler::getTransModKeyMask( bool trans1d, int groupidx ) const
{
    if ( trans1d )
	return planedragger_->get1DTranslateModKeyMask( groupidx );

    return planedragger_->get2DTranslateModKeyMask( groupidx );
}


//============================================================================

#define mZValue(idx) ( (idx%4)<2 ? zrange_.start : zrange_.stop )


RandomTrackDragger::RandomTrackDragger()
    : VisualObjectImpl( true )
    , horborder_( -mUdf(double), -mUdf(double), mUdf(double), mUdf(double) )
    , zborder_( -mUdf(float), mUdf(float) )
    , motion( this )
    , movefinished( this )
    , rightclicknotifier_( this )
    , rightclickeventinfo_( 0 )
    , displaytrans_( 0 )
    , panels_( new osg::Switch )
    , planedraggers_( new osg::Switch )
    , rotationaxis_( new osg::Switch )
    , showplanedraggers_( true )
    , planedraggerminsizeinsteps_( 1 )
    , zrange_( SI().zRange(true) )
    , showallpanels_( false )
    , postponepanelupdate_( false )
{
    for ( int dim=0; dim<3; dim++ )
	limits_[dim].setUdf();

    setMaterial( 0 );

    panels_->ref();
    panels_->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    panels_->getStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    panels_->getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    addChild( panels_ );
    setPanelsPolygonOffset( true );

    planedraggers_->ref();
    planedraggers_->getOrCreateStateSet()->setRenderingHint(
					    osg::StateSet::TRANSPARENT_BIN );
    planedraggers_->getStateSet()->setAttributeAndModes(
		new osg::PolygonOffset(-1.0,-1.0),
		osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    planedraggers_->getStateSet()->setAttributeAndModes(
		new osg::Material,
		osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    addChild( planedraggers_ );

    rotationaxis_->ref();
    rotationaxis_->getOrCreateStateSet()->setMode( GL_LIGHTING,
						   osg::StateAttribute::OFF );
    rotationaxis_->getStateSet()->setAttributeAndModes(
		new osg::LineWidth(2.0),
		osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    rotationaxis_->getStateSet()->setAttributeAndModes(
		new osg::PolygonOffset(-1.0,-1.0),
		osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    addChild( rotationaxis_ );

    setTransDragKeys( true, OD::ControlButton, 1 );
}


RandomTrackDragger::~RandomTrackDragger()
{
    detachAllNotifiers();
    deepErase( dragcontrols_ );

    panels_->unref();
    planedraggers_->unref();
    rotationaxis_->unref();

    deepUnRef( draggers_ );

    while ( !planedraghandlers_.isEmpty() )
	removePlaneDraggerCBHandler( 0 );

    if ( displaytrans_ ) displaytrans_->unRef();
}


void RandomTrackDragger::setDisplayTransformation( const mVisTrans* trans )
{
    if ( displaytrans_ )
    {
	displaytrans_->unRef();
	displaytrans_ = 0;
    }

    displaytrans_ = trans;

    if ( displaytrans_ )
	displaytrans_->ref();

    for ( int idx=0; idx<draggers_.size(); idx++ )
	draggers_[idx]->setDisplayTransformation( trans );

    updatePlaneDraggers();
}


const mVisTrans* RandomTrackDragger::getDisplayTransformation() const
{ return displaytrans_; }


void RandomTrackDragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<VisID>* RandomTrackDragger::rightClickedPath() const
{ return rightclickeventinfo_ ? &rightclickeventinfo_->pickedobjids : 0; }


const EventInfo* RandomTrackDragger::rightClickedEventInfo() const
{ return rightclickeventinfo_; }


void RandomTrackDragger::followActiveDragger( int activeidx )
{
    if ( !draggers_.validIdx(activeidx) )
	return;

    const Coord3 newpos = draggers_[activeidx]->getPos();
    if ( activeidx%2 )
    {
	const int subdraggeridx = activeidx%4;
	if ( subdraggeridx < 2 )
	    zrange_.start = newpos.z;
	else
	    zrange_.stop = newpos.z;

	for ( int idx=subdraggeridx; idx<draggers_.size(); idx+=4 )
	{
	    Coord3 pos = draggers_[idx]->getPos();
	    pos.z = mZValue( idx );

	    draggers_[idx-1]->setPos( pos );
	    if ( idx != activeidx )
		draggers_[idx]->setPos( pos );
	}
    }
    else
    {
	const int knotidx = activeidx/4;
	for ( int idx=4*knotidx; idx<=4*knotidx+3; idx++ )
	{
	    if ( idx != activeidx )
		draggers_[idx]->setPos( Coord3(newpos, mZValue(idx)) );
	}

    }
    updatePanels();
}


void RandomTrackDragger::startCB( CallBacker* cb )
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( cb == draggers_[idx] )
	{
	    setPanelsPolygonOffset( idx%2 );
	    draggers_[idx-2*(idx%2)+1]->turnOn( false );
	    return;
	}
    }
}


void RandomTrackDragger::moveCB( CallBacker* cb )
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( cb == draggers_[idx] )
	{
	    followActiveDragger( idx );

	    const int knotidx = idx/4;
	    motion.trigger( knotidx );
	    return;
	}
    }
}


void RandomTrackDragger::snapToLimits( Coord3& pos ) const
{
    for ( int dim=0; dim<3; dim++ )
    {
	if ( !mIsUdf(limits_[dim].start) && !mIsUdf(limits_[dim].step) )
	    pos[dim] = limits_[dim].snap( pos[dim] );
    }
}


void RandomTrackDragger::finishCB( CallBacker* cb )
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( cb == draggers_[idx] )
	{
	    setPanelsPolygonOffset( true );

	    Coord3 newpos = draggers_[idx]->getPos();
	    snapToLimits( newpos );
	    draggers_[idx]->setPos( newpos );

	    followActiveDragger( idx );
	    draggers_[idx-2*(idx%2)+1]->turnOn( true );
	    movefinished.trigger();
	    return;
	}
    }
}


int RandomTrackDragger::nrKnots() const
{
    return draggers_.size()/4;
}


Coord RandomTrackDragger::getKnot( int knotidx ) const
{
    if ( !draggers_.validIdx(4*knotidx) )
	return Coord::udf();

    return draggers_[4*knotidx]->getPos();
}


int RandomTrackDragger::getKnotIdx( const TypeSet<VisID>& pickpath ) const
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( pickpath.isPresent(draggers_[idx]->id()) )
	    return idx/4;
    }

    return -1;
}


void RandomTrackDragger::doSetKnot( int knotidx, const Coord& pos )
{
    if ( !draggers_.validIdx(4*knotidx) )
	return;

    for ( int idx=4*knotidx; idx<=4*knotidx+3; idx++ )
	draggers_[idx]->setPos( Coord3(pos, mZValue(idx)) );

    updatePanels();
}


void RandomTrackDragger::setKnot( int knotidx, const Coord& pos )
{
    for ( int idx=knotidx-1; idx<=knotidx; idx++ )
    {
	if ( !planedraghandlers_.validIdx(idx) )
	    continue;
	if ( planedraghandlers_[idx]->isMoving() )
	    return;
    }

    doSetKnot( knotidx, pos );
}


void RandomTrackDragger::insertKnot( int knotidx, const Coord& pos )
{
    if ( knotidx<0 || 4*knotidx>draggers_.size() )
	return;

    for ( int idx=4*knotidx; idx<=4*knotidx+3; idx++ )
    {
	Dragger* dragger = visBase::Dragger::create();
	dragger->ref();
	dragger->setSize( 20 );
	dragger->setDisplayTransformation( displaytrans_ );
	dragger->setSpaceLimits( limits_[0], limits_[1], limits_[2] );
	dragger->handleEvents( isHandlingEvents() );
	mAttachCB( dragger->started, RandomTrackDragger::startCB );
	mAttachCB( dragger->motion, RandomTrackDragger::moveCB );
	mAttachCB( dragger->finished, RandomTrackDragger::finishCB );

	visBase::MarkerSet* marker = visBase::MarkerSet::create();
	MarkerStyle3D markerstyle;

	marker->setMinimumScale( 0 );
	marker->setAutoRotateMode( visBase::MarkerSet::NO_ROTATION );
	marker->addPos( Coord3( 0, 0, 0 ) );
	marker->setMarkerResolution( 0.8f );

	if ( idx%2 )
	{
	    dragger->setDraggerType( Dragger::Translate1D );
	    dragger->setRotation( Coord3(0,1,0), M_PI_2 ); // confirms default
	    markerstyle = MarkerStyle3D::Cylinder;
	    markerstyle.size_ = 4;
	    marker->setMarkerHeightRatio( 3 );
	    marker->setRotationForAllMarkers( Coord3(0,1,0), M_PI_2 );
	}
	else
	{
	    dragger->setDraggerType( Dragger::Translate2D );
	    dragger->setRotation( Coord3(1,0,0), 0 );	  // nullifies default
	    markerstyle = MarkerStyle3D::Cube;
	    markerstyle.size_ = 12;
	    marker->setMarkerHeightRatio( 0.1 );
	    marker->setRotationForAllMarkers( Coord3(1,0,0), M_PI_2 );
	}

	marker->setMarkerStyle( markerstyle );
	dragger->setOwnShape( marker, false );
	dragger->setPos( Coord3(pos, mZValue(idx)) );	// Must be set last
	draggers_.insertAt( dragger, idx );
	draggermarkers_.insertAt( marker, idx );

	addChild( dragger->osgNode() );
    }

    updateKnotColor( knotidx, false );
    showadjacents_.insert( knotidx, false );
    updatePanels();
}


void RandomTrackDragger::removeKnot( int knotidx )
{
    if ( !draggers_.validIdx(4*knotidx) )
	return;

    for ( int idx=4*knotidx+3; idx>=4*knotidx; idx-- )
    {
	removeChild( draggers_[idx]->osgNode() );
	mDetachCB( draggers_[idx]->started, RandomTrackDragger::startCB );
	mDetachCB( draggers_[idx]->motion, RandomTrackDragger::moveCB );
	mDetachCB( draggers_[idx]->finished, RandomTrackDragger::finishCB );
	draggers_[idx]->unRef();
	draggers_.removeSingle( idx );
	draggermarkers_.removeSingle( idx );
    }

    showadjacents_.removeSingle( knotidx );
    updatePanels();
}


void RandomTrackDragger::setLimits( const Coord3& start, const Coord3& stop,
				    const Coord3& step )
{
    for ( int dim=0; dim<3; dim++ )
	limits_[dim] = StepInterval<float>( start[dim], stop[dim], step[dim] );

    for ( int idx=0; idx<draggers_.size(); idx ++ )
	draggers_[idx]->setSpaceLimits( limits_[0], limits_[1], limits_[2] );

    // Handle undefined borders as incredibly far borders
    horborder_.setTopLeft( start.coord() );
    horborder_.setBottomRight( stop.coord() );
    if ( limits_[0].step < 0.0 )
	horborder_.swapHor();
    if ( limits_[1].step < 0.0 )
	horborder_.swapVer();
    if ( mIsUdf(horborder_.left()) )
	horborder_.setLeft( -mUdf(double) );
    if ( mIsUdf(horborder_.top()) )
	horborder_.setTop( -mUdf(double) );
    zborder_.set( start.z, stop.z );
    if ( limits_[2].step < 0.0 )
	zborder_.set( stop.z, start.z );
    if ( mIsUdf(zborder_.start) )
	zborder_.start = -mUdf(float);


    // Correct meaningless zero-width intervals
    horborder_.sortCorners();
    zborder_.sort();
}

void RandomTrackDragger::updateZLimit( const Interval<float>& zborder )
{
    zborder_ = zborder;
}


void RandomTrackDragger::setDepthRange( const Interval<float>& rg )
{
    zrange_ = rg.isRev()==zrange_.isRev() ? rg :
					    Interval<float>(rg.stop,rg.start);

    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	Coord3 pos = draggers_[idx]->getPos();
	pos.z = mZValue( idx );
	draggers_[idx]->setPos( pos );
    }

    updatePanels();
}


Interval<float> RandomTrackDragger::getDepthRange() const
{
    Interval<float> zrg = zrange_;
    zrg.sort();
    return zrg;
}


void RandomTrackDragger::turnPanelOn( int planeidx, bool yn )
{
    if ( planeidx>=0 && planeidx<panels_->getNumChildren() )
	panels_->setValue( planeidx, yn );

    if ( planedraghandlers_.validIdx(planeidx) )
	planedraghandlers_[planeidx]->showDraggerBorder ( !yn );
}


void RandomTrackDragger::showAdjacentPanels( int knotidx, bool yn )
{
    if ( showadjacents_.validIdx(knotidx) && showadjacents_[knotidx]!=yn )
    {
	if ( knotidx>0 )
	{
	    if ( yn || (!showallpanels_ && !showadjacents_[knotidx-1]) )
		turnPanelOn( knotidx-1, yn );
	}

	if ( knotidx<showadjacents_.size()-1 )
	{
	    if ( yn || (!showallpanels_ && !showadjacents_[knotidx+1]) )
		turnPanelOn( knotidx, yn );
	}

	showadjacents_[knotidx] = yn;
    }
}


bool RandomTrackDragger::areAdjacentPanelsShown( int knotidx ) const
{
    if ( showadjacents_.validIdx(knotidx) )
	return showadjacents_[knotidx];

    return false;
}


void RandomTrackDragger::showAllPanels( bool yn )
{
    if ( showallpanels_ != yn )
    {
	for ( int idx=0; idx<showadjacents_.size()-1; idx++ )
	{
	    if ( yn || (!showadjacents_[idx] && !showadjacents_[idx+1]) )
		turnPanelOn( idx, yn );
	}

	showallpanels_ = yn;
    }
}


bool RandomTrackDragger::areAllPanelsShown() const
{ return showallpanels_; }


void RandomTrackDragger::postponePanelUpdate( bool yn )
{
    postponepanelupdate_ = yn;
    if ( !yn )
	updatePanels();
}


void RandomTrackDragger::updatePanels()
{
    if ( postponepanelupdate_ )
	return;

    if ( panels_->getNumChildren() >= nrKnots() )
	panels_->removeChildren( 0, panels_->getNumChildren()-nrKnots()-1 );

    for ( int knotidx=0; knotidx<nrKnots()-1; knotidx++ )
    {
	bool changed = false;

	if ( knotidx >= panels_->getNumChildren() )
	{
	    changed = true;

	    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	    geode->setNodeMask( Math::SetBits( geode->getNodeMask(),
				    cDraggerIntersecTraversalMask(), false) );
	    panels_->addChild( geode.get(), true );

	    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(4);
	    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array(1);

	    // 0: semi-transparent grey plane, 1: vertical green side poles
	    for ( int geomidx=0; geomidx<2; geomidx++ )
	    {
		osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
		osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

		if ( !geomidx )
		{
		    colors->push_back( osg::Vec4(0.7,0.7,0.7,0.5) );
		    geometry->getOrCreateStateSet()->setRenderingHint(
					    osg::StateSet::TRANSPARENT_BIN );
		}
		else
		    colors->push_back( osg::Vec4(0.0,0.7,0.0,1.0) );

		geometry->setVertexArray( vertices.get() );
		geometry->setNormalArray( normals.get() );
		geometry->setNormalBinding( osg::Geometry::BIND_OVERALL );
		geometry->setColorArray( colors.get() );
		geometry->setColorBinding( osg::Geometry::BIND_OVERALL );
		const GLenum primitive = geomidx ? GL_LINES : GL_QUADS;
		geometry->addPrimitiveSet( new osg::DrawArrays(primitive,0,4) );
		geode->addDrawable( geometry.get() );
	    }
	}

	turnPanelOn( knotidx, showallpanels_ || showadjacents_[knotidx]
					     || showadjacents_[knotidx+1] );

	osg::ref_ptr<osg::Geode> curgeode =
		    dynamic_cast<osg::Geode*>( panels_->getChild(knotidx) );
	osg::ref_ptr<osg::Geometry> curpanel =
		    dynamic_cast<osg::Geometry*>( curgeode->getDrawable(0) );
	osg::ref_ptr<osg::Vec3Array> verts =
		    dynamic_cast<osg::Vec3Array*>( curpanel->getVertexArray() );

	for ( int vtxidx=0; vtxidx<4; vtxidx++ )
	{
	    osg::Vec3 pos;
	    int idx = 4*knotidx;
	    idx += vtxidx==0 ? 0 : vtxidx==1 ? 2 : vtxidx==2 ? 6 : 4;
	    mVisTrans::transform(displaytrans_, draggers_[idx]->getPos(), pos);
	    if ( pos != (*verts)[vtxidx] )
	    {
		(*verts)[vtxidx] = pos;
		changed = true;
	    }
	}

	if ( changed )
	{
	    osg::ref_ptr<osg::Vec3Array> norms =
		    dynamic_cast<osg::Vec3Array*>( curpanel->getNormalArray() );

	    (*norms)[0] = ((*verts)[1]-(*verts)[0])^((*verts)[3]-(*verts)[0]);
	    (*norms)[0].normalize();
	    if ( zrange_.isRev() )
		(*norms)[0] = -(*norms)[0];

	    verts->dirty();
	    norms->dirty();
	    curpanel->dirtyGLObjects();
	    curpanel->dirtyBound();

	    osg::ref_ptr<osg::Geometry> curpoles =
		    dynamic_cast<osg::Geometry*>( curgeode->getDrawable(1) );
	    curpoles->dirtyGLObjects();
	    curpoles->dirtyBound();
	}
    }

    updatePlaneDraggers();
}


void RandomTrackDragger::showPlaneDraggers( bool yn, int minsteps )
{
    showplanedraggers_ = yn;
    planedraggerminsizeinsteps_ = abs( minsteps );
    updatePlaneDraggers();
}


void RandomTrackDragger::addPlaneDraggerCBHandler()
{
    PlaneDragCBHandler* pdcbh = new PlaneDragCBHandler( *this );
    pdcbh->ref();
    pdcbh->osgDragger().addDraggerCallback( pdcbh );
    planedraghandlers_ += pdcbh;
    planedraggers_->addChild( &pdcbh->osgDragger(), true );
}


void RandomTrackDragger::removePlaneDraggerCBHandler( int idx )
{
    if ( planedraghandlers_.validIdx(idx) )
    {
	planedraggers_->removeChild( idx, 1 );
	PlaneDragCBHandler* pdcbh = planedraghandlers_.removeSingle( idx );
	pdcbh->osgDragger().removeDraggerCallback( pdcbh );
	pdcbh->unref();
    }
}


void RandomTrackDragger::updatePlaneDraggers()
{
    while ( !planedraghandlers_.isEmpty() &&
	    planedraghandlers_.size()>=nrKnots() )
    {
	removePlaneDraggerCBHandler( 0 );
    }

    bool showverticalknotdraggers = true;

    for ( int knotidx=0; knotidx<nrKnots()-1; knotidx++ )
    {
	if ( knotidx >= planedraghandlers_.size() )
	    addPlaneDraggerCBHandler();

	const bool ok = planedraghandlers_[knotidx]->setCorners(
					    draggers_[knotidx*4]->getPos(),
					    draggers_[knotidx*4+6]->getPos() );
	bool horoverlap = !ok;
	const bool show = ok && canShowPlaneDragger(knotidx,horoverlap);

	if ( show )
	    showverticalknotdraggers = false;

	planedraghandlers_[knotidx]->showDraggerTabs( show );
	planedraghandlers_[knotidx]->showDraggerBorder(
					show && !panels_->getValue(knotidx) );

	if ( !show && knotidx<panels_->getNumChildren() )
	    panels_->setValue( knotidx, true );

	if ( !planedraghandlers_[knotidx]->isMoving() )
	    planedraggers_->setValue( knotidx, show  );

	updateKnotColor( knotidx+1, false );
	if ( !knotidx )
	    updateKnotColor( knotidx, false );

	if ( horoverlap )
	{
	    updateKnotColor( knotidx+1, true );
	    updateKnotColor( knotidx, true );
	}
    }

    for ( int idx=1; idx<4*nrKnots(); idx+=2 )
    {
	if ( !draggers_[idx]->isMoving() )
	    draggers_[idx]->turnOn( showverticalknotdraggers );
    }
}


bool RandomTrackDragger::canShowPlaneDragger( int planeidx,
					      bool& horoverlap ) const
{
    const Coord3 planeboxsteps = getPlaneBoundingBoxInSteps( planeidx );
    if ( mIsUdf(planeboxsteps) )
	return false;

    const float threshold = mMAX( 0.0, planedraggerminsizeinsteps_-0.5 );

    if ( planeboxsteps.coord().sqAbs() <= threshold*threshold )
	horoverlap = true;

    if ( !showplanedraggers_ )
	return false;
    if ( !planedraggerminsizeinsteps_  )
	return true;
    if ( horoverlap )
	return false;

    return planeboxsteps.z > threshold;
}


void RandomTrackDragger::updateKnotColor( int knotidx, bool horoverlap )
{
    if ( !draggermarkers_.validIdx(4*knotidx) )
	return;

    for ( int idx=4*knotidx+3; idx>=4*knotidx; idx-- )
    {
	OD::Color markercol( 0, 153, 0 );
	if ( idx%2==0 )
	    markercol = horoverlap ? OD::Color(255,0,0) : OD::Color(0,204,0);

	draggermarkers_[idx]->setMarkersSingleColor( markercol );
	draggers_[idx]->setArrowColor( OD::Color(0,255,0) );
    }
}


bool RandomTrackDragger::doesKnotStickToBorder( int knotidx ) const
{
    if ( nrKnots()<2 || (knotidx>0 && knotidx<nrKnots()-1) )
	return false;

    const char firstflags  = getOnBorderFlags( knotidx ? nrKnots()-1 : 0 );
    const char secondflags = getOnBorderFlags( knotidx ? nrKnots()-2 : 1 );

    if ( firstflags==1 || firstflags==2 || firstflags==4 || firstflags==8 )
	return !(firstflags & secondflags);

    return firstflags;
}


unsigned char RandomTrackDragger::getOnBorderFlags( int knotidx ) const
{
    unsigned char flags = 0;
    if ( !draggers_.validIdx(4*knotidx) )
	return flags;

    Coord3 pos = draggers_[4*knotidx]->getPos();
    snapToLimits( pos );

    const float eps = 1e-6;
    const float threshold0 = mIsUdf(limits_[0].step) ?
					    eps : 0.5*fabs(limits_[0].step);
    const float threshold1 = mIsUdf(limits_[1].step) ?
					    eps : 0.5*fabs(limits_[1].step);

    if ( fabs(horborder_.left()  -pos.x) < threshold0 ) flags += 1;
    if ( fabs(horborder_.top()	 -pos.y) < threshold1 ) flags += 2;
    if ( fabs(horborder_.right() -pos.x) < threshold0 ) flags += 4;
    if ( fabs(horborder_.bottom()-pos.y) < threshold1 ) flags += 8;

    return flags;
}


Coord3 RandomTrackDragger::getPlaneBoundingBoxInSteps( int planeidx ) const
{
    Coord3 res( Coord3::udf() );
    if ( !draggers_.validIdx(4*planeidx+6) )
	return res;

    Coord3 topleft = draggers_[planeidx*4]->getPos();
    snapToLimits( topleft );
    Coord3 botright = draggers_[planeidx*4+6]->getPos();
    snapToLimits( botright );

    for ( int dim=0; dim<3; dim++ )
    {
	const float eps = 1e-6;
	float step = limits_[dim].step;
	if ( mIsUdf(step) || !step )
	    step = eps;

	res[dim] = fabs( botright[dim]-topleft[dim] ) / fabs(step);
    }
    return res;
}


void RandomTrackDragger::showRotationAxis( bool yn, int planeidx,
					   Coord normpickedpos )
{
    while ( rotationaxis_->getNumChildren() )
	rotationaxis_->removeChild( 0, 1 );

    setPanelsPolygonOffset( !yn );

    if ( !yn || !draggers_.validIdx(4*planeidx+6) || mIsUdf(normpickedpos) )
	return;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    rotationaxis_->addChild( geode.get(), true );
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(4);
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    colors->push_back( osg::Vec4(0.0,1.0,0.0,1.0) );
    geometry->setVertexArray( vertices.get() );
    geometry->setColorArray( colors.get() );
    geometry->setColorBinding( osg::Geometry::BIND_OVERALL );
    geometry->addPrimitiveSet( new osg::DrawArrays(GL_LINES,0,4) );
    geode->addDrawable( geometry.get() );

    const Coord3 topleft = draggers_[planeidx*4]->getPos();
    const Coord3 botright = draggers_[planeidx*4+6]->getPos();

    if ( !SI().isRightHandSystem() )
	normpickedpos.y = 1.0-normpickedpos.y;

    Coord3 pivot = topleft*(1.0-normpickedpos.x) + botright*normpickedpos.x;
    pivot.z = topleft.z*(1.0-normpickedpos.y) + botright.z*normpickedpos.y;

    Coord3 pos( pivot );
    pos.z = 1.1*topleft.z - 0.1*botright.z;
    mVisTrans::transform( displaytrans_, pos, (*vertices)[0] );
    const float pivotoffset = 0.05 * (botright.z-topleft.z);
    pos.z = pivot.z - pivotoffset;
    mVisTrans::transform( displaytrans_, pos, (*vertices)[1] );
    pos.z = pivot.z + pivotoffset;
    mVisTrans::transform( displaytrans_, pos, (*vertices)[2] );
    pos.z = 1.1*botright.z - 0.1*topleft.z;
    mVisTrans::transform( displaytrans_, pos, (*vertices)[3] );
}


void RandomTrackDragger::setPanelsPolygonOffset( bool yn )
{
    panels_->getOrCreateStateSet()->removeAttribute(
					osg::StateAttribute::POLYGONOFFSET );
    if ( yn )
    {
	panels_->getOrCreateStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(-0.5,-0.5),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    }
}


RandomTrackDragger::DragControl::DragControl( bool trans1d, int groupidx )
    : trans1d_( trans1d )
    , groupidx_( groupidx )
    , mousebutmask_( osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON )
    , modkeymask_( osgGA::GUIEventAdapter::NONE )
{}


#define mRank(trans1d,groupidx) (trans1d + 2*groupidx)

int RandomTrackDragger::getDragControlIdx( bool trans1d, int groupidx,
					   bool docreate )
{
    const int newrank = mRank(trans1d,groupidx);

    int idx = 0;
    while ( idx<dragcontrols_.size() )
    {
	const int rank = mRank( dragcontrols_[idx]->trans1d_,
				dragcontrols_[idx]->groupidx_ );
	if ( rank >= newrank )
	{
	    if ( rank == newrank )
		return idx;

	    break;
	}
	idx++;
    }

    if ( docreate )
    {
	dragcontrols_.insertAt( new DragControl(trans1d,groupidx), idx );
	return idx;
    }

    return -1;
}


int RandomTrackDragger::getDragControlIdx( bool trans1d, int groupidx ) const
{
    return const_cast<RandomTrackDragger*>(this)->getDragControlIdx(
						    trans1d, groupidx, false );
}


void RandomTrackDragger::setTransDragKeys( bool trans1d,
					   int keys, int groupidx )
{
    int mask = osgGA::GUIEventAdapter::NONE;

    if ( keys & OD::ControlButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( keys & OD::ShiftButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( keys & OD::AltButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    const int dragcontrolidx = getDragControlIdx( trans1d, groupidx, true );
    dragcontrols_[dragcontrolidx]->modkeymask_ = mask;

    for ( int idx=0; idx<planedraghandlers_.size(); idx++ )
	planedraghandlers_[idx]->setTransModKeyMask( trans1d, mask, groupidx );
}


int RandomTrackDragger::getTransDragKeys( bool trans1d, int groupidx ) const
{
    int mask = osgGA::GUIEventAdapter::NONE;

    const int dragcontrolidx = getDragControlIdx( trans1d, groupidx );

    if ( dragcontrolidx >= 0 )
	mask = dragcontrols_[dragcontrolidx]->modkeymask_;
    else if ( !planedraghandlers_.isEmpty() )
	mask = planedraghandlers_[0]->getTransModKeyMask( trans1d, groupidx );

    int state = OD::NoButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_CTRL )
	state |= OD::ControlButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_SHIFT )
	state |= OD::ShiftButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_ALT )
	state |= OD::AltButton;

    return (OD::ButtonState) state;
}


void RandomTrackDragger::handleEvents( bool yn )
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
	draggers_[idx]->handleEvents( yn );

    for ( int idx=0; idx<planedraghandlers_.size(); idx++ )
	planedraghandlers_[idx]->osgDragger().setHandleEvents( yn );
}


bool RandomTrackDragger::isHandlingEvents() const
{
    return draggers_.isEmpty() ? true : draggers_[0]->isHandlingEvents();
}


} // namespace visBase
