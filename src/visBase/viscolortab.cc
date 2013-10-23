/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viscolortab.h"

#include "coltabsequence.h"
#include "coltabmapper.h"
#include "coltabindex.h"
#include "dataclipper.h"
#include "iopar.h"
#include "scaler.h"
#include "visdataman.h"
#include "valseries.h"
#include "math2.h"

mCreateFactoryEntry( visBase::VisColorTab );

namespace visBase
{

const char* VisColorTab::sKeyColorSeqID()	{ return "ColorSeq ID"; }
const char* VisColorTab::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* VisColorTab::sKeyRange()		{ return "Range"; }
const char* VisColorTab::sKeyClipRate()		{ return "Cliprate"; }
const char* VisColorTab::sKeyAutoScale()	{ return "Auto scale"; }
const char* VisColorTab::sKeySymmetry()		{ return "Symmetry"; }
const char* VisColorTab::sKeySymMidval()	{ return "Symmetry Midvalue"; }

static const int cNrColors = 255;

VisColorTab::VisColorTab()
    : sequencechange(this)
    , rangechange(this)
    , autoscalechange(this)
    , viscolseq_(0)
    , indextable_(0)
    , ctmapper_( new ColTab::Mapper )
{
    setColorSeq( ColorSequence::create() );
}


VisColorTab::~VisColorTab()
{
    if ( viscolseq_ )
    {
	viscolseq_->change.remove( mCB(this,VisColorTab,colorseqChanged) );
	viscolseq_->unRef();
    }

    delete indextable_;
    delete ctmapper_;
}


bool VisColorTab::autoScale() const
{ return ctmapper_->setup_.type_ == ColTab::MapperSetup::Auto; }


void VisColorTab::setAutoScale( bool yn )
{
    if ( yn == autoScale() ) return;

    ctmapper_->setup_.type_ = yn ? ColTab::MapperSetup::Auto
				 : ColTab::MapperSetup::Fixed;
    autoscalechange.trigger();
}


void VisColorTab::setSymMidval( float symmidval )
{
    if (  mIsEqual(symmidval,ctmapper_->setup_.symmidval_,mDefEps) ) return;

    ctmapper_->setup_.symmidval_ = symmidval;
    ctmapper_->update( true );
    rangechange.trigger();
}


float VisColorTab::symMidval() const
{ return ctmapper_->setup_.symmidval_; }


const Interval<float>& VisColorTab::clipRate() const
{ return ctmapper_->setup_.cliprate_; }


void VisColorTab::setClipRate( const Interval<float>& ncr )
{
    if ( mIsEqual(ncr.start,ctmapper_->setup_.cliprate_.start,mDefEps) ||
      	 mIsEqual(ncr.stop,ctmapper_->setup_.cliprate_.stop,mDefEps) )
	return;

    ctmapper_->setup_.cliprate_ = ncr;
    ctmapper_->update( false );
    rangechange.trigger();
}


void VisColorTab::scaleTo( const float* values, od_int64 nrvalues )
{
    float* valuesnc = const_cast<float*>(values);
    const ArrayValueSeries<float,float>* arrvs =
	new ArrayValueSeries<float,float>( valuesnc, false, nrvalues );
    scaleTo( arrvs, nrvalues );
}


void VisColorTab::scaleTo( const ValueSeries<float>* values, od_int64 nrvalues )
{
    ctmapper_->setData( values, nrvalues );
    rangechange.trigger();
}


Color VisColorTab::color( float val ) const
{
    return indextable_->color( val );
}


void VisColorTab::setNrSteps( int nrsteps )
{
    indextable_->setNrCols( nrsteps );
    indextable_->update();
}


int VisColorTab::nrSteps() const
{
    return indextable_->nrCols();
}


int VisColorTab::colIndex( float val ) const
{
    if ( !Math::IsNormalNumber(val) || mIsUdf(val) )
	return nrSteps();
    return indextable_->indexForValue( val );
}


Color VisColorTab::tableColor( int idx ) const
{
    return idx==nrSteps()
    	? viscolseq_->colors().undefColor()
	: indextable_->colorForIndex( idx );  
}


void VisColorTab::scaleTo( const Interval<float>& rg )
{
    ctmapper_->setRange( rg );
    rangechange.trigger();
}


const Interval<float>& VisColorTab::getInterval() const
{ return ctmapper_->range(); }


void VisColorTab::setColorSeq( ColorSequence* ns )
{
    if ( viscolseq_ )
    {
	viscolseq_->change.remove( mCB(this,VisColorTab,colorseqChanged) );
	viscolseq_->unRef();
    }

    viscolseq_ = ns;
    viscolseq_->ref();

    delete indextable_;
    indextable_ = new ColTab::IndexedLookUpTable( viscolseq_->colors(),
	    					  cNrColors, ctmapper_ );

    viscolseq_->change.notify( mCB(this,VisColorTab,colorseqChanged) );
    sequencechange.trigger();
}


const ColorSequence& VisColorTab::colorSeq() const
{ return *viscolseq_; }


ColorSequence& VisColorTab::colorSeq()
{ return *viscolseq_; }


void VisColorTab::colorseqChanged( CallBacker* )
{
    indextable_->update();
    sequencechange.trigger();
}


void VisColorTab::triggerRangeChange()
{ rangechange.trigger(); }


void VisColorTab::triggerSeqChange()
{ sequencechange.trigger(); }


void VisColorTab::triggerAutoScaleChange()
{ autoscalechange.trigger(); }



}; // namespace visBase
