/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "visannot.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vispickstyle.h"
#include "vismaterial.h"
#include "ranges.h"
#include "samplingdata.h"
#include "axislayout.h"
#include "iopar.h"
#include "survinfo.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoNormal.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSwitch.h"

#include "SoOD.h"
#include "SoOneSideRender.h"

#include <osg/Geode>
#include <osg/Geometry>

mCreateFactoryEntry( visBase::Annotation );

namespace visBase
{

const char* Annotation::textprefixstr()	    { return "Text "; }
const char* Annotation::cornerprefixstr()   { return "Corner "; }
const char* Annotation::showtextstr()	    { return "Show Text"; }
const char* Annotation::showscalestr()	    { return "Show Scale"; }

Annotation::Annotation()
    : VisualObjectImpl( false )
    , coords_(new SoCoordinate3)
    , textswitch_(new SoSwitch)
    , scaleswitch_(new SoSwitch)
    , gridlineswitch_(new SoSwitch)
    , pickstyle_(PickStyle::create())
    , texts_(0)
    , geode_( doOsg() ? new osg::Geode : 0 )
{
    annotscale_[0] = annotscale_[1] = annotscale_[2] = 1;

    annotcolor_ = Color::White();
    pickstyle_->ref();
    addChild( pickstyle_->getInventorNode() );
    pickstyle_->setStyle( PickStyle::Unpickable );

    addChild( coords_ );

    if ( doOsg() )
    {
	float pos[8][3] =
	{
	     { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 },
	     { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 1 }, { 1, 1, 0 }
	};

	const osg::Vec3* ptr = (osg::Vec3*) pos;
	osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array( 8, ptr );
	osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

	geometry->setVertexArray( coords );

	GLubyte indices[] = { 0, 1, 1, 2, 2, 3, 3, 0,
	     		       4, 5, 5, 6, 6, 7, 7, 4,
			       0, 4, 1, 5, 2, 6, 3, 7 };
	geometry->addPrimitiveSet(
		new osg::DrawElementsUByte( GL_LINES, 24, indices  ) );
	
	geometry->setColorBinding( osg::Geometry::BIND_OVERALL );

	geode_->addDrawable( geometry );
	addChild( geode_ );
    }
    float pos[8][3] =
    {
	{ 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 },
	{ 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }
    };

    coords_->point.setValues( 0, 8, pos );

    SoIndexedLineSet* line = new SoIndexedLineSet;
    addChild( line );
    line->setName( "Survey box" );

    int coordidx = 0;
    int indexes[] = { 0, 1, 2, 3, 0, -1 };
    line->coordIndex.setValues( coordidx, 6, indexes );
    coordidx += 6;

    indexes[0] = 4; indexes[1] = 5; indexes[2] = 6; indexes[3] = 7;
    indexes[4] = 4;
    line->coordIndex.setValues( coordidx, 6, indexes );
    coordidx += 6;

    indexes[0] = 0; indexes[1] = 4; indexes[2] = -1;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 1; indexes[1] = 5;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 2; indexes[1] = 6;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 3; indexes[1] = 7;
    line->coordIndex.setValues( coordidx, 2, indexes );

    addChild( textswitch_ );
    texts_ = DataObjectGroup::create();
    texts_->setSeparate( false );
    texts_->ref();
    textswitch_->addChild( texts_->getInventorNode() );
    textswitch_->whichChild = 0;

#define mAddText \
    text = Text2::create(); text->setJustification( Text2::Right ); \
    texts_->addObject( text );

    Text2* text = 0; mAddText mAddText mAddText

    addChild( scaleswitch_ );
    scaleswitch_->whichChild = SO_SWITCH_ALL;

    DataObjectGroup* scale = DataObjectGroup::create();
    scale->ref();
    scale->setSeparate( false );
    scaleswitch_->addChild( scale->getInventorNode() );
    scales_ += scale;
    
    scale = DataObjectGroup::create();
    scale->setSeparate( false );
    scale->ref();
    scaleswitch_->addChild( scale->getInventorNode() );
    scales_ += scale;
    
    scale = DataObjectGroup::create();
    scale->setSeparate( false );
    scale->ref();
    scaleswitch_->addChild( scale->getInventorNode() );
    scales_ += scale;

    addChild( gridlineswitch_ );
    gridlineswitch_->whichChild = SO_SWITCH_ALL;

    gridlinecoords_ = new SoCoordinate3;
    gridlineswitch_->addChild( gridlinecoords_ );

    onesiderender_ = new SoOneSideRender;
    gridlineswitch_->addChild( onesiderender_ );
    onesiderender_->nodes.removeNode( 0 );

    for ( int idx=0; idx<6; idx++ )
    {
	SoIndexedLineSet* gridline = new SoIndexedLineSet;
	gridlines_ += gridline;
	onesiderender_->nodes.addNode( gridline );
    }

    onesiderender_->normals.set1Value( 0, SbVec3f( 0,  0,  1 ) );
    onesiderender_->normals.set1Value( 1, SbVec3f( 0,  0, -1 ) );
    onesiderender_->normals.set1Value( 2, SbVec3f( 0,  1,  0 ) );
    onesiderender_->normals.set1Value( 3, SbVec3f( 0, -1,  0 ) );
    onesiderender_->normals.set1Value( 4, SbVec3f( 1,  0,  0 ) );
    onesiderender_->normals.set1Value( 5, SbVec3f( -1, 0,  0 ) );

    updateTextPos();
}


void Annotation::updateGridLines(int dim)
{
    SbVec3f p0;
    SbVec3f p1;
    getAxisCoords( dim, p0, p1 );
    Interval<float> range( p0[dim], p1[dim] );
    const SamplingData<float> sd = AxisLayout<float>( range ).sd_;

    SbVec3f corners[4];
    int gridlineidxs[4];
    if ( dim==0 )
    {
	corners[0] = coords_->point[ 0 ];
	corners[1] = coords_->point[ 3 ];
	corners[2] = coords_->point[ 7 ];
	corners[3] = coords_->point[ 4 ];
	gridlineidxs[0] = 0; gridlineidxs[1] = 3;
	gridlineidxs[2] = 1; gridlineidxs[3] = 2; 
    }
    else if ( dim==1 )
    {
	corners[0] = coords_->point[ 0 ];
	corners[1] = coords_->point[ 1 ];
	corners[2] = coords_->point[ 5 ];
	corners[3] = coords_->point[ 4 ];
	gridlineidxs[0] = 0; gridlineidxs[1] = 5;
	gridlineidxs[2] = 1; gridlineidxs[3] = 4; 
    }
    else
    {
	corners[0] = coords_->point[ 0 ];
	corners[1] = coords_->point[ 1 ];
	corners[2] = coords_->point[ 2 ];
	corners[3] = coords_->point[ 3 ];
	gridlineidxs[0] = 2; gridlineidxs[1] = 5;
	gridlineidxs[2] = 3; gridlineidxs[3] = 4; 
    }

    int ci = gridlinecoords_->point.getNum();

    for ( int idx=0; ; idx++ )
    {
	const float val = sd.atIndex(idx);
	if ( val <= range.start )	continue;
	else if ( val > range.stop )	break;

	corners[0][dim]=corners[1][dim]=corners[2][dim]=corners[3][dim] = val;
	gridlinecoords_->point.setValues( ci, 4, corners );
	SoIndexedLineSet* lineset;

#define mAddOneLine( c1, c2 ) \
	lineset = gridlines_[gridlineidxs[c1]]; \
	lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), ci+c1 ); \
	lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), ci+c2 ); \
	lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), -1 )

	mAddOneLine( 0, 1 );
	mAddOneLine( 1, 2 );
	mAddOneLine( 2, 3 );
	mAddOneLine( 3, 0 );

	ci += 4;
    }
}


Annotation::~Annotation()
{
    scales_[0]->unRef();
    scales_[1]->unRef();
    scales_[2]->unRef();
    texts_->unRef();
    pickstyle_->unRef();
}


void Annotation::showText( bool yn )
{
    textswitch_->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool Annotation::isTextShown() const
{
    return textswitch_->whichChild.getValue() == 0;
}


void Annotation::showScale( bool yn )
{
    scaleswitch_->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}


bool Annotation::isScaleShown() const
{
    return scaleswitch_->whichChild.getValue() != SO_SWITCH_NONE;
}


void Annotation::showGridLines( bool yn )
{
    gridlineswitch_->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
}


bool Annotation::isGridLinesShown() const
{
    return gridlineswitch_->whichChild.getValue() != SO_SWITCH_NONE;
}


void Annotation::setCubeSampling( const CubeSampling& cs )
{
    const Interval<int> inlrg = cs.hrg.inlRange();
    const Interval<int> crlrg = cs.hrg.crlRange();
    const Interval<float>& zrg = cs.zrg;

    setCorner( 0, inlrg.start, crlrg.start, zrg.start );
    setCorner( 1, inlrg.stop, crlrg.start, zrg.start );
    setCorner( 2, inlrg.stop, crlrg.stop, zrg.start );
    setCorner( 3, inlrg.start, crlrg.stop, zrg.start );
    setCorner( 4, inlrg.start, crlrg.start, zrg.stop );
    setCorner( 5, inlrg.stop, crlrg.start, zrg.stop );
    setCorner( 6, inlrg.stop, crlrg.stop, zrg.stop );
    setCorner( 7, inlrg.start, crlrg.stop, zrg.stop );

    onesiderender_->positions.set1Value( 0, SbVec3f( inlrg.center(),  crlrg.center(), zrg.start ) );
    onesiderender_->positions.set1Value( 1, SbVec3f( inlrg.center(),  crlrg.center(), zrg.stop ) );
    onesiderender_->positions.set1Value( 2, SbVec3f( inlrg.center(),  crlrg.start,  zrg.center() ) );
    onesiderender_->positions.set1Value( 3, SbVec3f( inlrg.center(), crlrg.stop,  zrg.center() ) );
    onesiderender_->positions.set1Value( 4, SbVec3f( inlrg.start,  crlrg.center(),  zrg.center() ) );
    onesiderender_->positions.set1Value( 5, SbVec3f( inlrg.stop, crlrg.center(),  zrg.center() ) );

    updateTextPos();
    updateGridLines();
}



void Annotation::setCorner( int idx, float x, float y, float z )
{
    if ( geode_ && geode_->getNumDrawables() )
    {
	 osg::ref_ptr<osg::Geometry> geometry =
	     (osg::Geometry*) geode_->getDrawable( 0 );

	 osg::Vec3& coord =
	     ((osg::Vec3*) geometry->getVertexArray()->getDataPointer())[idx];

	 coord = osg::Vec3f( x, y, z );
    }


    float c[3] = { x, y, z };
    coords_->point.setValues( idx, 1, &c );
}


Coord3 Annotation::getCorner( int idx ) const
{
    const SbVec3f pos = coords_->point[idx];
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


void Annotation::setText( int dim, const char* string )
{
    Text2* text = (Text2*)texts_->getObject( dim );
    if ( text )
	text->setText( string );
}


void Annotation::setTextColor( int dim, const Color& col )
{
    Text2* text = (Text2*)texts_->getObject( dim );
    if ( text )
	text->getMaterial()->setColor( col );
}


void Annotation::updateTextColor( const Color& col )
{
    annotcolor_ = col;
    for ( int idx=0; idx<3; idx++ )
    {
	setTextColor( idx, annotcolor_ );
    }
    updateTextPos();
}


void Annotation::updateTextPos()
{
    updateTextPos( 0 );
    updateTextPos( 1 );
    updateTextPos( 2 );
}


void Annotation::updateGridLines()
{
    if ( !gridlines_.size() )
	return;

    for ( int idx=0; idx<gridlines_.size(); idx++ )
	gridlines_[idx]->coordIndex.setNum( 0 );

    gridlinecoords_->point.setNum( 0 );

    updateGridLines( 0 );
    updateGridLines( 1 );
    updateGridLines( 2 );

}


void Annotation::getAxisCoords( int dim, SbVec3f& p0, SbVec3f& p1 ) const
{
    int pidx0;
    int pidx1;

    if ( dim==0)
    {
	pidx0 = 0;
	pidx1 = 1;
    }
    else if ( dim==1 )
    {
	pidx0 = 0;
	pidx1 = 3;
    }
    else
    {
	pidx0 = 0;
	pidx1 = 4;
    }

    p0 = coords_->point[pidx0];
    p1 = coords_->point[pidx1];
}


void Annotation::updateTextPos( int dim )
{
    SbVec3f p0;
    SbVec3f p1;
    getAxisCoords( dim, p0, p1 );

    SbVec3f tp;
    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;

    ((Text2*)texts_->getObject(dim))
			->setPosition( Coord3(tp[0],tp[1],tp[2]) );

    Interval<float> range( p0[dim], p1[dim] );
    const SamplingData<float> sd = AxisLayout<float>( range ).sd_;
    scales_[dim]->removeAll();

    for ( int idx=0; ; idx++ )
    {
	float val = sd.atIndex(idx);
	if ( val <= range.start )	continue;
	else if ( val > range.stop )	break;

	Text2* text = Text2::create();
	scales_[dim]->addObject( text );
	Coord3 pos( p0[0], p0[1], p0[2] );
	pos[dim] = val;
	float displayval = val;
	displayval *= annotscale_[dim];

	text->setPosition( pos );
	text->setText( toString(displayval) );
	text->getMaterial()->setColor( annotcolor_ );
    }
}


void visBase::Annotation::setAnnotScale( int dim, int nv )
{
    annotscale_[dim] = nv;
    updateTextPos( dim );
}


Text2* visBase::Annotation::getText( int dim, int textnr )
{
    DataObjectGroup* group = 0;
    group = scales_[dim];
    mDynamicCastGet(Text2*,text,group->getObject(textnr));
    return text;
}


void Annotation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	Coord3 pos = getCorner( idx );
	par.set( key, pos.x, pos.y, pos.z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;
	Text2* text = (Text2*)texts_->getObject( idx );
	if ( !text ) continue;

	par.set( key, (const char*)text->getText() );
    }

    par.setYN( showtextstr(), isTextShown() );
    par.setYN( showscalestr(), isScaleShown() );
}


int Annotation::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;

	double x, y, z;
	if ( !par.get( key, x, y, z ) )
	    return -1;

	setCorner( idx, x, y, z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;

	const char* text = par.find( key );
	if ( !text ) return -1;

	setText( idx, text );
    }

    bool yn = true;
    par.getYN( showtextstr(), yn );
    showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    return 1;
}

}; //namespace
