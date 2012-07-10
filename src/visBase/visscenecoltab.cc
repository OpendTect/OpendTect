/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: visscenecoltab.cc,v 1.26 2012-07-10 08:05:39 cvskris Exp $";

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "axislayout.h"
#include "scaler.h"

#include "LegendKit.h"
#include <Inventor/SbColor.h>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl( false )
    , legendkit_(new LegendKit)
    , flipseq_( false )
{
    addChild( legendkit_ );
    legendkit_->ref();
    legendkit_->size = SbVec2s(20,150);
    legendkit_->setDiscreteMode( true );
    legendkit_->enableBackground( false );
    setPos( SceneColTab::BottomLeft );
    setLegendColor( Color(170,170,170) );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( legendkit_ );
    legendkit_->unref();
}


void SceneColTab::setLegendColor( const Color& col )
{
#define col2f(rgb) float(col.rgb())/255
    legendkit_->setTickAndLinesColor( SbColor(col2f(r),col2f(g),col2f(b)) );
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
    legendkit_->size[0] = w;
    legendkit_->size[1] = h;
    updateVis();
}


void SceneColTab::setPos( Pos pos )
{
    pos_ = pos; 

    if ( pos_ ==  TopLeft )
    {  
	legendkit_->istop = true;
	legendkit_->isleft = true;
    }
    else if ( pos_ == TopRight )
    {
	legendkit_->istop = true;
	legendkit_->isleft = false;
    }
    else if ( pos_ == BottomLeft )
    {
        legendkit_->istop = false;
        legendkit_->isleft = true;
    }
    else if ( pos_ == BottomRight )
    {
	legendkit_->istop = false;
        legendkit_->isleft = false;
    }
    updateVis();
}


Geom::Size2D<int> SceneColTab::getSize()
{
    Geom::Size2D<int> sz;
    sz.setWidth( legendkit_->size[0] );
    sz.setHeight( legendkit_->size[1] );
    return sz;
}


void SceneColTab::updateVis()
{
    if ( !isOn() )
	return;

    const int nrcols = 256;
    legendkit_->clearColors();
    ColTab::IndexedLookUpTable table( sequence_, nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
    {
	Color col = table.colorForIndex( flipseq_ ? nrcols-idx-1 : idx );
	od_uint32 val = ( (unsigned int)(col.r()&0xff) << 24 ) |
		        ( (unsigned int)(col.g()&0xff) << 16 ) |
		        ( (unsigned int)(col.b()&0xff) <<  8 ) |
		        (col.t()&0xff);
	legendkit_->addDiscreteColor( double(idx)/256., val );
    }
    
    legendkit_->clearTicks();
    AxisLayout<float> al; al.setDataRange( rg_ );
    LinScaler scaler( rg_.start, 0, rg_.stop, 1 );

    const int upto = abs( mNINT32((al.stop_-al.sd_.start)/al.sd_.step) );
    for ( int idx=0; idx<=upto; idx++ )
    {
	const float val = al.sd_.start + idx*al.sd_.step;
	const float normval = scaler.scale( val );
	if ( normval>=0 && normval<=1 )
	    legendkit_->addBigTick( normval, val );
    }

    legendkit_->minvalue = toString( rg_.start );
    legendkit_->maxvalue = toString( rg_.stop );
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


void SceneColTab::turnOn( bool yn )
{
    VisualObjectImpl::turnOn( yn );
    updateVis();
}


} // namespace visBase
