/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolortab.cc,v 1.18 2003-11-07 12:22:02 bert Exp $";

#include "viscolortab.h"

#include "dataclipper.h"
#include "visdataman.h"
#include "scaler.h"
#include "colortab.h"
#include "iopar.h"

mCreateFactoryEntry( visBase::VisColorTab );

const char* visBase::VisColorTab::colorseqidstr = "ColorSeq ID";
const char* visBase::VisColorTab::scalefactorstr = "Scale Factor";
const char* visBase::VisColorTab::clipratestr = "Cliprate";
const char* visBase::VisColorTab::autoscalestr = "Auto scale";


visBase::VisColorTab::VisColorTab()
    : sequencechange( this )
    , rangechange( this )
    , autoscalechange( this )
    , colseq( 0 )
    , scale( *new LinScaler )
    , autoscale( true )
    , cliprate( 0.025 )
{
    setColorSeq( ColorSequence::create() );
}


visBase::VisColorTab::~VisColorTab()
{
    if ( colseq )
    {
	colseq->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	colseq->unRef();
    }
}


bool visBase::VisColorTab::autoScale() const
{ return autoscale; }


void visBase::VisColorTab::setAutoScale( bool yn )
{
    if ( yn==autoscale ) return;

    autoscale = yn;
    if ( autoscale ) autoscalechange.trigger();
}


float visBase::VisColorTab::clipRate() const
{
    return cliprate;
}


void visBase::VisColorTab::setClipRate( float ncr )
{
    if ( mIS_ZERO(ncr-cliprate) ) return;

    cliprate = ncr;
    if ( autoscale ) autoscalechange.trigger();
}


void visBase::VisColorTab::scaleTo( float* values, int nrvalues )
{
    DataClipper clipper( cliprate );
    clipper.setApproxNrValues( nrvalues, 5000 );
    clipper.putData( values, nrvalues );
    clipper.calculateRange();
    scaleTo( clipper.getRange() );
}


Color  visBase::VisColorTab::color( float val ) const
{
    return colseq->colors().color( scale.scale( val ), false );
}


void visBase::VisColorTab::setNrSteps( int idx )
{
    return colseq->colors().calcList( idx );
}


int visBase::VisColorTab::nrSteps() const
{
    return colseq->colors().nrColors();
}


int visBase::VisColorTab::colIndex( float val ) const
{
    if ( mIsUndefined(val) ) return nrSteps();
    return colseq->colors().colorIdx( scale.scale( val ), nrSteps() );
}


Color visBase::VisColorTab::tableColor( int idx ) const
{
    return idx==nrSteps()
    	? colseq->colors().undefcolor : colseq->colors().tableColor(idx);
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

	rangechange.trigger();
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
	colseq->unRef();
    }

    colseq = ns;
    colseq->ref();
    colseq->change.notify( mCB( this, VisColorTab, colorseqchanged ));
    sequencechange.trigger();
}


const visBase::ColorSequence& visBase::VisColorTab::colorSeq() const
{ return *colseq; }


visBase::ColorSequence& visBase::VisColorTab::colorSeq()
{ return *colseq; }


void visBase::VisColorTab::colorseqchanged()
{
    sequencechange.trigger();
}


void visBase::VisColorTab::triggerRangeChange()
{ rangechange.trigger(); }


void visBase::VisColorTab::triggerSeqChange()
{ sequencechange.trigger(); }


void visBase::VisColorTab::triggerAutoScaleChange()
{ autoscalechange.trigger(); }


int visBase::VisColorTab::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int colseqid;
    if ( !par.get( colorseqidstr, colseqid ) )
	return -1;

    visBase::DataObject* dataobj = visBase::DM().getObj(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(visBase::ColorSequence*,cs,dataobj);
    if ( !cs ) return -1;

    setColorSeq( cs );

    float cliprate_ = 0.025;
    par.get( clipratestr, cliprate_ );
    setClipRate( cliprate_ );

    bool autoscale_ = true;
    par.getYN( autoscalestr, autoscale_ );
    setAutoScale( autoscale_ );

    const char* scalestr = par.find( scalefactorstr );
    if ( !scalestr ) return -1;

    scale.fromString( scalestr );
    return 1;
}


void visBase::VisColorTab::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( colorseqidstr, colseq->id() );
    if ( saveids.indexOf(colseq->id())==-1 ) saveids += colseq->id();
    par.set( scalefactorstr, scale.toString() );
    par.set( clipratestr, cliprate );
    par.setYN( autoscalestr, autoscale );
}
