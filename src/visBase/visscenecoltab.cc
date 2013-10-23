/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "axislayout.h"
#include "scaler.h"

#include <osg/Geode>
#include <osg/Geometry>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl( false )
    , geode_( new osg::Geode )
    , flipseq_( false )
{
    addChild( geode_ );
    osg::Geometry* geom = new osg::Geometry;
    geode_->addDrawable( geom );
    osg::Vec3Array* coords = new osg::Vec3Array;
    geom->setVertexArray( coords );
    coords->push_back( osg::Vec3(0,0,0));
    coords->push_back( osg::Vec3(0,1,0));
    coords->push_back( osg::Vec3(1,0,0));
    coords->push_back( osg::Vec3(1,1,0));

    setLegendColor( Color(170,170,170) );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( geode_ );
}


void SceneColTab::setLegendColor( const Color& col )
{
#define col2f(rgb) float(col.rgb())/255
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    if ( sequence_==ctseq )
	return;
    
    sequence_ = ctseq;
    updateVis();
}


void SceneColTab::setSize( int w, int h )
{
    updateVis();
}


void SceneColTab::setPos( Pos pos )
{
    pos_ = pos; 

        updateVis();
}


Geom::Size2D<int> SceneColTab::getSize()
{
    Geom::Size2D<int> sz;
    return sz;
}


void SceneColTab::updateVis()
{
    if ( !isOn() )
	return;

    const int nrcols = 256;
    //legendkit_->clearColors();
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
    {
	//Color col = table.colorForIndex( flipseq_ ? nrcols-idx-1 : idx );
	//od_uint32 val = ( (unsigned int)(col.r()&0xff) << 24 ) |
	//	        ( (unsigned int)(col.g()&0xff) << 16 ) |
	//	        ( (unsigned int)(col.b()&0xff) <<  8 ) |
	//	        (col.t()&0xff);
	//legendkit_->addDiscreteColor( double(idx)/256., val );
    }
    
    //legendkit_->clearTicks();
    AxisLayout<float> al; al.setDataRange( rg_ );
    LinScaler scaler( rg_.start, 0, rg_.stop, 1 );

    const int upto = abs( mNINT32((al.stop_-al.sd_.start)/al.sd_.step) );
    for ( int idx=0; idx<=upto; idx++ )
    {
	//const float val = al.sd_.start + idx*al.sd_.step;
	//const float normval = (float) scaler.scale( val );
	//if ( normval>=0 && normval<=1 )
	  //  legendkit_->addBigTick( normval, val );
    }

    //legendkit_->minvalue = toString( rg_.start );
    //legendkit_->maxvalue = toString( rg_.stop );
}


void SceneColTab::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg = ms.range_;
    if ( rg==rg_ && flipseq_==ms.flipseq_ )
	return;
    
    rg_ = rg;
    flipseq_ = ms.flipseq_;

    updateVis();
}


bool SceneColTab::turnOn( bool yn )
{
    const bool res = VisualObjectImpl::turnOn( yn );
    updateVis();
    return res;
}


} // namespace visBase
