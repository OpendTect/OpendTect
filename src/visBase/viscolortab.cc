/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscolortab.cc,v 1.21 2005-02-04 14:31:34 kristofer Exp $";

#include "viscolortab.h"

#include "dataclipper.h"
#include "visdataman.h"
#include "scaler.h"
#include "colortab.h"
#include "iopar.h"

namespace visBase
{

mCreateFactoryEntry( VisColorTab );

const char* VisColorTab::colorseqidstr = "ColorSeq ID";
const char* VisColorTab::scalefactorstr = "Scale Factor";
const char* VisColorTab::clipratestr = "Cliprate";
const char* VisColorTab::autoscalestr = "Auto scale";


VisColorTab::VisColorTab()
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


VisColorTab::~VisColorTab()
{
    if ( colseq )
    {
	colseq->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	colseq->unRef();
    }

    delete &scale;
}


bool VisColorTab::autoScale() const
{ return autoscale; }


void VisColorTab::setAutoScale( bool yn )
{
    if ( yn==autoscale ) return;

    autoscale = yn;
    if ( autoscale ) autoscalechange.trigger();
}


float VisColorTab::clipRate() const
{
    return cliprate;
}


void VisColorTab::setClipRate( float ncr )
{
    if ( mIsEqual(ncr,cliprate,mDefEps) ) return;

    cliprate = ncr;
    if ( autoscale ) autoscalechange.trigger();
}


void VisColorTab::scaleTo( float* values, int nrvalues )
{
    DataClipper clipper( cliprate );
    clipper.setApproxNrValues( nrvalues, 5000 );
    clipper.putData( values, nrvalues );
    clipper.calculateRange();
    scaleTo( clipper.getRange() );
}


Color  VisColorTab::color( float val ) const
{
    return colseq->colors().color( scale.scale( val ), false );
}


void VisColorTab::setNrSteps( int idx )
{
    return colseq->colors().calcList( idx );
}


int VisColorTab::nrSteps() const
{
    return colseq->colors().nrColors();
}


int VisColorTab::colIndex( float val ) const
{
    if ( mIsUndefined(val) ) return nrSteps();
    return colseq->colors().colorIdx( scale.scale( val ), nrSteps() );
}


Color VisColorTab::tableColor( int idx ) const
{
    return idx==nrSteps()
    	? colseq->colors().undefcolor : colseq->colors().tableColor(idx);
}


void VisColorTab::scaleTo( const Interval<float>& rg )
{
    float width = rg.width();
    if ( mIsZero(width,mDefEps) )
	scaleTo( Interval<float>(rg.start -1, rg.start+1));
    else
    {
	scale.factor = 1.0/rg.width();
	if ( rg.start > rg.stop ) scale.factor *= -1;
	scale.constant = -rg.start*scale.factor;

	rangechange.trigger();
    }
}


Interval<float> VisColorTab::getInterval() const
{
    float start = -scale.constant / scale.factor;
    float stop = start + 1 / scale.factor;
    return Interval<float>(start,stop);
}


void VisColorTab::setColorSeq( ColorSequence* ns )
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


const ColorSequence& VisColorTab::colorSeq() const
{ return *colseq; }


ColorSequence& VisColorTab::colorSeq()
{ return *colseq; }


void VisColorTab::colorseqchanged()
{
    sequencechange.trigger();
}


void VisColorTab::triggerRangeChange()
{ rangechange.trigger(); }


void VisColorTab::triggerSeqChange()
{ sequencechange.trigger(); }


void VisColorTab::triggerAutoScaleChange()
{ autoscalechange.trigger(); }


int VisColorTab::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int colseqid;
    if ( !par.get( colorseqidstr, colseqid ) )
	return -1;

    DataObject* dataobj = DM().getObject(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(ColorSequence*,cs,dataobj);
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


void VisColorTab::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( colorseqidstr, colseq->id() );
    if ( saveids.indexOf(colseq->id())==-1 ) saveids += colseq->id();
    par.set( scalefactorstr, scale.toString() );
    par.set( clipratestr, cliprate );
    par.setYN( autoscalestr, autoscale );
}

}; // namespace visBase
