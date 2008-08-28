/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: visscenecoltab.cc,v 1.1 2008-08-28 08:53:30 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visscenecoltab.h"

#include "coltabindex.h"
#include "coltabsequence.h"
#include "legendkit.h"

namespace visBase
{

SceneColTab::SceneColTab()
    : DataObject()
    , legendkit_(new LegendKit)
{
    addChild( legendkit_ );
}


SceneColTab::~SceneColTab()
{
    removeChild( legendkit_ );
    delete legendkit_;
}


void SceneColTab::setColTabSequence( const ColTab::Sequence& ctseq )
{
    legendkit_->clearData();
    ColTab::IndexedLookUpTable table( ctseq, 64 );
    for ( int idx=0; idx<63; idx++ )
    {
	Color col = table.colorForIndex( idx );
	legendkit_->addDiscreteColor( double((idx+1)/64), col.rgb() );
    }
}


void SceneColTab::setRange( const Interval<float>& rg )
{
}



} // namespace visBase
