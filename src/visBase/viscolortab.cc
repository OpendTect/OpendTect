/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Mar 2002
 RCS:           $Id: viscolortab.cc,v 1.36 2007-10-11 12:19:55 cvsraman Exp $
________________________________________________________________________

-*/

#include "viscolortab.h"

#include "colortab.h"
#include "dataclipper.h"
#include "iopar.h"
#include "scaler.h"
#include "visdataman.h"

mCreateFactoryEntry( visBase::VisColorTab );

namespace visBase
{

const char* VisColorTab::sKeyColorSeqID()	{ return "ColorSeq ID"; }
const char* VisColorTab::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* VisColorTab::sKeyClipRate()		{ return "Cliprate"; }
const char* VisColorTab::sKeyAutoScale()	{ return "Auto scale"; }


VisColorTab::VisColorTab()
    : sequencechange( this )
    , rangechange( this )
    , autoscalechange( this )
    , colseq_( 0 )
    , scale_( *new LinScaler )
    , autoscale_( true )
    , cliprate_( ColorTable::defPercClip()/100 )
{
    setColorSeq( ColorSequence::create() );
}


VisColorTab::~VisColorTab()
{
    if ( colseq_ )
    {
	colseq_->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	colseq_->unRef();
    }

    delete &scale_;
}


bool VisColorTab::autoScale() const
{ return autoscale_; }


void VisColorTab::setAutoScale( bool yn )
{
    if ( yn==autoscale_ ) return;

    autoscale_ = yn;
    if ( autoscale_ ) autoscalechange.trigger();
}


bool VisColorTab::isSymmetric() const
{
    return symmetry_;
}


void VisColorTab::setSymmetric( bool yn ) 
{
    symmetry_ = yn;
}


float VisColorTab::clipRate() const
{
    return cliprate_;
}


void VisColorTab::setClipRate( float ncr )
{
    if ( mIsEqual(ncr,cliprate_,mDefEps) ) return;

    cliprate_ = ncr;
    if ( autoscale_ ) autoscalechange.trigger();
}


void VisColorTab::scaleTo( const float* values, int nrvalues )
{
    DataClipper clipper;
    clipper.setApproxNrValues( nrvalues, 5000 );
    clipper.putData( values, nrvalues );
    Interval<float> range( 0, 1 );
    clipper.calculateRange( cliprate_, range );
    scaleTo( range );
}


void VisColorTab::scaleTo( const ValueSeries<float>& values, int nrvalues )
{
    DataClipper clipper;
    clipper.setApproxNrValues( nrvalues, 5000 );
    clipper.putData( values, nrvalues );
    Interval<float> range( 0, 1 );
    clipper.calculateRange( cliprate_, range );
    scaleTo( range );
}


Color  VisColorTab::color( float val ) const
{
    return colseq_->colors().color( scale_.scale( val ), false );
}


void VisColorTab::setNrSteps( int idx )
{
    return colseq_->colors().calcList( idx );
}


int VisColorTab::nrSteps() const
{
    return colseq_->colors().nrColors();
}


int VisColorTab::colIndex( float val ) const
{
    if ( mIsUdf(val) )
	return nrSteps();
    return colseq_->colors().colorIdx( scale_.scale( val ), nrSteps() );
}


Color VisColorTab::tableColor( int idx ) const
{
    return idx==nrSteps()
    	? colseq_->colors().undefcolor_ : colseq_->colors().tableColor(idx);
}


void VisColorTab::scaleTo( const Interval<float>& rg )
{
    Interval<float> valrg = rg;
    const float width = valrg.width();
    if ( mIsZero(width,mDefEps) )
    {
	valrg.start--;
	valrg.stop++;
    }

    scale_.factor = 1.0/valrg.width();
    if ( valrg.start > valrg.stop ) scale_.factor *= -1;
    scale_.constant = -valrg.start*scale_.factor;

    rangechange.trigger();
}


Interval<float> VisColorTab::getInterval() const
{
    const float start = -scale_.constant / scale_.factor;
    const float stop = start + 1 / scale_.factor;
    return Interval<float>(start,stop);
}


void VisColorTab::setColorSeq( ColorSequence* ns )
{
    if ( colseq_ )
    {
	colseq_->change.remove( mCB( this, VisColorTab, colorseqchanged ));
	colseq_->unRef();
    }

    colseq_ = ns;
    colseq_->ref();
    colseq_->change.notify( mCB( this, VisColorTab, colorseqchanged ));
    sequencechange.trigger();
}


const ColorSequence& VisColorTab::colorSeq() const
{ return *colseq_; }


ColorSequence& VisColorTab::colorSeq()
{ return *colseq_; }


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
    if ( !par.get( sKeyColorSeqID(), colseqid ) )
	return -1;

    DataObject* dataobj = DM().getObject(colseqid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(ColorSequence*,cs,dataobj);
    if ( !cs ) return -1;

    setColorSeq( cs );

    float cliprate = ColorTable::defPercClip()/100;
    par.get( sKeyClipRate(), cliprate );
    setClipRate( cliprate );

    bool autoscale = true;
    par.getYN( sKeyAutoScale(), autoscale );
    setAutoScale( autoscale );

    const char* scalestr = par.find( sKeyScaleFactor() );
    if ( !scalestr ) return -1;

    scale_.fromString( scalestr );
    return 1;
}


void VisColorTab::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    par.set( sKeyColorSeqID(), colseq_->id() );
    if ( saveids.indexOf(colseq_->id())==-1 ) saveids += colseq_->id();
    par.set( sKeyScaleFactor(), scale_.toString() );
    par.set( sKeyClipRate(), cliprate_ );
    par.setYN( sKeyAutoScale(), autoscale_ );
}

}; // namespace visBase
