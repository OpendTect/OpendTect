/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolortab.cc,v 1.1 2002-03-11 10:46:03 kristofer Exp $";

#include "viscolortab.h"
#include "visdataman.h"
#include "scaler.h"
#include "colortab.h"

visBase::VisColorTab::VisColorTab()
    : change( this )
    , colseq( 0 )
    , scale( *new LinScaler )
{
    setColorSeq( ColorSequence::create() );
}


visBase::VisColorTab::~VisColorTab()
{
    colseq->unRef();
}


Color  visBase::VisColorTab::color( float val ) const
{
    return colseq->colors().color( scale.scale( val ), false );
}


void visBase::VisColorTab::scaleTo( const Interval<float>& rg )
{
    scale.factor = 1.0/rg.width();
    scale.constant = -rg.start*scale.factor;

    change.trigger();
}


void visBase::VisColorTab::setColorSeq( ColorSequence* ns )
{
    if ( colseq )
    {
	colseq->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	visBase::DataManager::manager.unRef( colseq );
    }

    colseq = ns;
    visBase::DataManager::manager.ref( colseq );
    colseq->change.notify( mCB( this, VisColorTab, colorseqchanged ));
    change.trigger();
}


void visBase::VisColorTab::colorseqchanged()
{
    change.trigger();
}




