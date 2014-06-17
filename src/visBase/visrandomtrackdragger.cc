/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visrandomtrackdragger.h"

#include "visdragger.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vistransform.h"
#include "survinfo.h"

#include <osg/Switch>
#include <osg/PolygonOffset>
#include <osg/Geometry>
#include <osg/Geode>

mCreateFactoryEntry( visBase::RandomTrackDragger );

namespace visBase
{


#define mZValue(idx) ( (idx%4)<2 ? zrange_.start : zrange_.stop )


RandomTrackDragger::RandomTrackDragger()
    : VisualObjectImpl( true )
    , motion( this )
    , rightclicknotifier_( this )
    , rightclickeventinfo_( 0 )
    , displaytrans_( 0 )
    , panels_( new osg::Switch )
    , zrange_( SI().zRange(true) )
    , showallpanels_( false )
{
    for ( int dim=0; dim<3; dim++ )
	limits_[dim] = StepInterval<float>::udf();

    setMaterial( 0 );

    panels_->ref();

    panels_->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    panels_->getStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    panels_->getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    panels_->getStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(-1.0,-1.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );

    panels_->getStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );

    addChild( panels_ );
}


RandomTrackDragger::~RandomTrackDragger()
{
    panels_->unref();
    detachAllNotifiers();
    deepUnRef( draggers_ );

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
}


const mVisTrans* RandomTrackDragger::getDisplayTransformation() const
{ return displaytrans_; }


void RandomTrackDragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<int>* RandomTrackDragger::rightClickedPath() const
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


void RandomTrackDragger::finishCB( CallBacker* cb )
{
    for ( int idx=0; idx<draggers_.size(); idx++ )
    {
	if ( cb == draggers_[idx] )
	{
	    Coord3 newpos = draggers_[idx]->getPos();
	    for ( int dim=0; dim<3; dim++ )
	    {
		if ( !mIsUdf(limits_[dim].start) && !mIsUdf(limits_[dim].step) )
		    newpos[dim] = limits_[dim].snap( newpos[dim] );
	    }
	    draggers_[idx]->setPos( newpos );

	    followActiveDragger( idx );
	    draggers_[idx-2*(idx%2)+1]->turnOn( true );
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


void RandomTrackDragger::setKnot( int knotidx, const Coord& pos )
{
    if ( !draggers_.validIdx(4*knotidx) )
	return;

    for ( int idx=4*knotidx; idx<=4*knotidx+3; idx++ )
	draggers_[idx]->setPos( Coord3(pos, mZValue(idx)) );

    updatePanels();
}


void RandomTrackDragger::insertKnot( int knotidx, const Coord& pos )
{
    if ( knotidx<0 || 4*knotidx>draggers_.size() )
	return;

    for ( int idx=4*knotidx; idx<=4*knotidx+3; idx++ )
    {
	Dragger* dragger = visBase::Dragger::create();
	dragger->ref();
	dragger->setArrowColor( Color(0,255,0) );
	dragger->setSize( 20 );
	dragger->setDisplayTransformation( displaytrans_ );
	dragger->setSpaceLimits( limits_[0], limits_[1], limits_[2] );
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
	    dragger->setRotation( Coord3(0,1,0), M_PI_2 );
	    marker->setMarkersSingleColor( Color(0,153,0) );
	    markerstyle = MarkerStyle3D::Cylinder;
	    markerstyle.size_ = 4;
	    marker->setMarkerHeightRatio( 3 );
	    marker->setMarkerRotation( Coord3(0,1,0), M_PI_2 );
	}
	else
	{
	    dragger->setDraggerType( Dragger::Translate2D );
	    dragger->setRotation( Coord3(1,0,0), M_PI_2 );
	    marker->setMarkersSingleColor( Color(0,204,0) );
	    markerstyle = MarkerStyle3D::Cube;
	    markerstyle.size_ = 8;
	    marker->setMarkerHeightRatio( 0.1 );
	    marker->setMarkerRotation( Coord3(1,0,0), M_PI_2 );
	}

	marker->setMarkerStyle( markerstyle );
	dragger->setOwnShape( marker, false );
	dragger->setPos( Coord3(pos, mZValue(idx)) );	// Must be set last
	draggers_.insertAt( dragger, idx );

	addChild( dragger->osgNode() );
    }

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
}


void RandomTrackDragger::setDepthRange( const Interval<float>& rg )
{
    zrange_ = rg;

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


void RandomTrackDragger::showAdjacentPanels( int knotidx, bool yn )
{
    if ( showadjacents_.validIdx(knotidx) && showadjacents_[knotidx]!=yn )
    {
	if ( knotidx>0 )
	{
	    if ( yn || (!showallpanels_ && !showadjacents_[knotidx-1]) )
		panels_->setValue( knotidx-1, yn );
	}

	if ( knotidx<showadjacents_.size()-1 )
	{
	    if ( yn || (!showallpanels_ && !showadjacents_[knotidx+1]) )
		panels_->setValue( knotidx, yn );
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
		panels_->setValue( idx, yn );
	}

	showallpanels_ = yn;
    }
}


bool RandomTrackDragger::areAllPanelsShown() const
{ return showallpanels_; }


void RandomTrackDragger::updatePanels()
{
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

	panels_->setValue( knotidx, showallpanels_ ||
				    showadjacents_[knotidx] ||
				    showadjacents_[knotidx+1] );

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
	    curpanel->dirtyDisplayList();
	    curpanel->dirtyBound();

	    osg::ref_ptr<osg::Geometry> curpoles =
		    dynamic_cast<osg::Geometry*>( curgeode->getDrawable(1) );
	    curpoles->dirtyDisplayList();
	    curpoles->dirtyBound();
	}
    }
}


}; // namespace visBase
