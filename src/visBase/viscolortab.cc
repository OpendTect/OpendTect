/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolortab.cc,v 1.6 2002-04-26 13:00:08 kristofer Exp $";

#include "viscolortab.h"
#include "visdataman.h"
#include "scaler.h"
#include "colortab.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::VisColorTab );

const char* visBase::VisColorTab::colorseqidstr = "ColorSeq ID";
const char* visBase::VisColorTab::scalefactorstr = "Scale Factor";

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
    float width = rg.width();
    if ( mIS_ZERO(width) )
	scaleTo( Interval<float>(rg.start -1, rg.start+1));
    else
    {
	scale.factor = 1.0/rg.width();
	if ( rg.start > rg.stop ) scale.factor *= -1;
	scale.constant = -rg.start*scale.factor;

	change.trigger();
    }
}


Interval<float> visBase::VisColorTab::getInterval() const
{
    float start = -scale.constant / scale.factor;
    float stop = start + 1 / scale.factor;
    return Interval<float>(start,stop);
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


int visBase::VisColorTab::usePar( const IOPar& par )
{
    int colseqid;
    if ( !par.get( colorseqidstr, colseqid ) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::ColorSequence*,cs,dataobj);
    if ( !cs ) return -1;

    setColorSeq( cs );

    const char* scalestr = par.find( scalefactorstr );
    if ( !scalestr ) return -1;

    scale.fromString( scalestr );
    return 1;
}


void visBase::VisColorTab::fillPar( IOPar& par ) const
{
    par.set( colorseqidstr, colseq->id() );
    par.set( scalefactorstr, scale.toString() );
}

