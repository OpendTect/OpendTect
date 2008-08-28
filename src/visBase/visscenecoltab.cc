/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: visscenecoltab.cc,v 1.2 2008-08-28 12:22:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabsequence.h"
#include "viscolortab.h"
#include "coltabsequence.h"
#include "LegendKit.h"

#include <Inventor/SbColor.h>

mCreateFactoryEntry( visBase::SceneColTab );

namespace visBase
{

SceneColTab::SceneColTab()
    : VisualObjectImpl()
    , legendkit_(new LegendKit)
{
    addChild( legendkit_ );
    legendkit_->ref();
    legendkit_->setDiscreteMode( true );
    legendkit_->enableBackground( false );
    legendkit_->description.setValue( 0 );
    legendkit_->setTickAndLinesColor( SbColor(170./255,170./255,170./255) );
    setColTabSequence( ColTab::Sequence("") );
}


SceneColTab::~SceneColTab()
{
    removeChild( legendkit_ );
    legendkit_->unref();
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    const float nrcols = 256;
    legendkit_->clearData();
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
}


void SceneColTab::setColTabID( int id )
{
    mDynamicCastGet(VisColorTab*,coltab,DM().getObject(id))
    if ( coltab )
    {
	setColTabSequence( coltab->colorSeq().colors() );
	setRange( coltab->getInterval() );
    }
}

} // namespace visBase
