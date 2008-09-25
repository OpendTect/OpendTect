/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: visscenecoltab.cc,v 1.5 2008-09-25 09:44:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabsequence.h"
#include "linear.h"
#include "scaler.h"
#include "viscolortab.h"

#include "LegendKit.h"
#include <Inventor/SbColor.h>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl()
    , viscoltab_(0)
    , legendkit_(new LegendKit)
{
    addChild( legendkit_ );
    legendkit_->ref();
    legendkit_->setDiscreteMode( true );
    legendkit_->enableBackground( false );
    legendkit_->setTickAndLinesColor( SbColor(170./255,170./255,170./255) );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( legendkit_ );
    legendkit_->unref();

    setColTabID( -1 );
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    const int nrcols = 256;
    legendkit_->clearColors();
    ColTab::IndexedLookUpTable table( ctseq, nrcols );
    for ( int idx=0; idx<nrcols; idx++ )
    {
	Color col = table.colorForIndex( idx );
	od_uint32 val = ( (unsigned int)(col.r()&0xff) << 24 ) |
		        ( (unsigned int)(col.g()&0xff) << 16 ) |
		        ( (unsigned int)(col.b()&0xff) <<  8 ) |
		        (col.t()&0xff);
	legendkit_->addDiscreteColor( double(idx)/256., val );
    }
}


void SceneColTab::setRange( const Interval<float>& rg )
{
    legendkit_->clearTicks();
    AxisLayout al; al.setDataRange( rg );
    LinScaler scaler( rg.start, 0, rg.stop, 1 );

    int idx = 0;
    while ( true )
    {
	const float val = al.sd.start + idx*al.sd.step;
	if ( val > al.stop ) break;

	const float normval = scaler.scale( val );
	if ( normval>=0 && normval<=1 )
	    legendkit_->addBigTick( normval, val );
	idx++;
    }

    legendkit_->minvalue = BufferString(rg.start).buf();
    legendkit_->maxvalue = BufferString(rg.stop).buf();
}


void SceneColTab::setColTabID( int id )
{
    mDynamicCastGet(VisColorTab*,coltab,DM().getObject(id))
    if ( coltab == viscoltab_ )
	return;

    if ( viscoltab_ )
    {
	viscoltab_->rangechange.remove( mCB(this,SceneColTab,rangeChg) );
	viscoltab_->sequencechange.remove( mCB(this,SceneColTab,seqChg) );
	viscoltab_->unRef();
    }

    viscoltab_ = coltab;
    if ( viscoltab_ )
    {
	viscoltab_->ref();
	viscoltab_->rangechange.notify( mCB(this,SceneColTab,rangeChg) );
	viscoltab_->sequencechange.notify( mCB(this,SceneColTab,seqChg) );
	seqChg(0);
	rangeChg(0);
    }
}


void SceneColTab::seqChg( CallBacker* )
{ setColTabSequence( viscoltab_->colorSeq().colors() ); }

void SceneColTab::rangeChg( CallBacker* )
{ setRange( viscoltab_->getInterval() ); }

} // namespace visBase
