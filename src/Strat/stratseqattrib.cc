/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "strattransl.h"
#include "stratlayer.h"
#include "stratreftree.h"
#include "stratlayermodel.h"
#include "statruncalc.h"
#include "propertyref.h"
#include "ascstream.h"
#include "datapointset.h"
#include "separstr.h"
#include "keystrs.h"
#include "iopar.h"
#include "survinfo.h"
#include <math.h>


#define mFileType "Layer Sequence Attribute Set"
static const char* sKeyFileType = mFileType;
mDefSimpleTranslators(StratLayerSequenceAttribSet,mFileType,od,Attr);
DefineEnumNames(Strat::LaySeqAttrib,Transform,1,"Value Transformation")
{ "Power", "Log", "Exp", 0 };


Strat::LaySeqAttrib* Strat::LaySeqAttribSet::gtAttr( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( attr(idx).name() == nm )
	    return const_cast<Strat::LaySeqAttrib*>( (*this)[idx] );
    }
    return 0;
}


#define mDoIOPar(fn,ky,val) \
	iop.fn( IOPar::compKey(ky,idx), val )


void Strat::LaySeqAttribSet::putTo( IOPar& iop ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const LaySeqAttrib& lsa = attr( idx );
	mDoIOPar( set, sKey::Name(), lsa.name() );
	mDoIOPar( set, sKey::Property(), lsa.prop_.name() );
	mDoIOPar( setYN, LaySeqAttrib::sKeyIsLocal(), lsa.islocal_ );
	mDoIOPar( set, LaySeqAttrib::sKeyStats(), lsa.stat_ );
	if ( !lsa.islocal_ )
	{
	    mDoIOPar( set, LaySeqAttrib::sKeyUnits(), lsa.units_ );
	    mDoIOPar( set, LaySeqAttrib::sKeyLithos(), lsa.liths_ );
	}
	FileMultiString fms( LaySeqAttrib::TransformNames()[lsa.transform_] );
	if ( mIsUdf(lsa.transformval_) )
	    fms += sKey::FloatUdf();
	else
	    fms += lsa.transformval_;
	mDoIOPar( set, LaySeqAttrib::sKeyTransform(), fms );
    }
}


void Strat::LaySeqAttribSet::getFrom( const IOPar& iop )
{
    deepErase( *this );

    for ( int idx=0; ; idx++ )
    {
	const char* res = iop.find( IOPar::compKey(sKey::Property(),idx) );
	if ( !res || !*res ) break;

	const PropertyRef* pr = PROPS().find( res );
	if ( !pr && Strat::Layer::thicknessRef().name() == res )
	    pr = &Strat::Layer::thicknessRef();
	if ( !pr )
	    continue;
	BufferString nm; mDoIOPar( get, sKey::Name(), nm );
	if ( nm.isEmpty() || attr(nm) ) continue;

	LaySeqAttrib* lsa = new LaySeqAttrib( *this, *pr, nm );
	mDoIOPar( getYN, LaySeqAttrib::sKeyIsLocal(), lsa->islocal_ );
	mDoIOPar( get, LaySeqAttrib::sKeyStats(), lsa->stat_ );
	if ( !lsa->islocal_ )
	{
	    mDoIOPar( get, LaySeqAttrib::sKeyUnits(), lsa->units_ );
	    mDoIOPar( get, LaySeqAttrib::sKeyLithos(), lsa->liths_ );
	}
	const char* ky = IOPar::compKey( LaySeqAttrib::sKeyTransform(), idx );
	const FileMultiString fms( iop.find(ky) );
	const int sz = fms.size();
	mSetUdf( lsa->transformval_ );
	if ( sz > 1 )
	{
	    lsa->transform_ = LaySeqAttrib::parseEnumTransform( fms[0] );
	    lsa->transformval_ = toFloat( fms[1] );
	    if ( lsa->transform_==LaySeqAttrib::Log && lsa->transformval_<0 )
		mSetUdf( lsa->transformval_ );
	}
	*this += lsa;
    }
}


bool Strat::LaySeqAttribSet::getFrom( std::istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return false;

    IOPar iop; iop.getFrom( astrm );
    erase(); getFrom( iop );
    return true;
}


bool Strat::LaySeqAttribSet::putTo( std::ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	return false;

    IOPar iop; putTo( iop );
    iop.putTo( astrm );
    return strm.good();
}


Strat::LaySeqAttribCalc::LaySeqAttribCalc( const Strat::LaySeqAttrib& desc,
					   const Strat::LayerModel& lm )
    : attr_(desc)
    , validx_(-1)
{
    if ( attr_.islocal_ )
    {
	statupscl_ = (Stats::UpscaleType)Stats::UpscaleTypeDef()
					.convert( desc.stat_ );
	stattype_ = Stats::typeFor( statupscl_ );
    }
    else
    {
	stattype_ = (Stats::Type)Stats::TypeDef().convert( desc.stat_ );
	statupscl_ = Stats::upscaleTypeFor( stattype_ );
    }

    for ( int idx=0; idx<lm.propertyRefs().size(); idx++ )
    {
	if ( lm.propertyRefs()[idx] && 
		lm.propertyRefs()[idx]->name() == attr_.prop_.name() )
	    { validx_ = idx; break; }
    }
    if ( validx_ < 0 )
	return;

    if ( !attr_.islocal_ )
    {
	for ( int idx=0; idx<attr_.units_.size(); idx++ )
	{
	    const Strat::UnitRef* ref
			= lm.refTree().find( attr_.units_.get(idx) );
	    if ( ref ) units_ += ref;
	}
	for ( int idx=0; idx<attr_.liths_.size(); idx++ )
	{
	    const Strat::Lithology* lith = lm.refTree().lithologies().get(
					    attr_.liths_.get(idx) );
	    if ( lith ) liths_ += lith;
	}
    }
}


float Strat::LaySeqAttribCalc::getValue( const LayerSequence& seq,
					 const Interval<float>& zpos ) const
{
    return attr_.islocal_ ? getLocalValue( seq, zpos ) : getGlobalValue(seq);
}


float Strat::LaySeqAttribCalc::getLocalValue( const LayerSequence& seq,
					  const Interval<float>& zrg ) const
{
    const ObjectSet<Layer>& lays = seq.layers();
    const int nrlays = lays.size();
    if ( validx_ < 0 || nrlays < 1 )
	return mUdf(float);

    int layidx = 0;
    while ( layidx < nrlays && lays[layidx]->zBot() <= zrg.start )
	layidx++;

    TypeSet<float> vals; TypeSet<float> wts;
    const float midz = zrg.center();
    for ( ; layidx < nrlays && lays[layidx]->zTop() <= zrg.stop; layidx++ )
    {
	const Strat::Layer& lay = *lays[layidx];
	Interval<float> insiderg( lay.zTop(), lay.zBot() );
	if ( statupscl_ == Stats::TakeNearest )
	{
	   if ( insiderg.includes(midz,true) )
		{ vals += lay.value(validx_); wts += 1; break; }
	   continue;
	}

	if ( insiderg.start < zrg.start ) insiderg.start = zrg.start;
	if ( insiderg.stop > zrg.stop ) insiderg.stop = zrg.stop;
	const float wt = insiderg.width();
	if ( wt < 1e-6 ) continue;
	const float val = lay.value( validx_ );
	if ( mIsUdf(val) ) continue;
	vals += val; wts += wt;
    }

    const int nrvals = vals.size();
    if ( nrvals < 1 )
	return mUdf(float);
    applyTransform( vals );
    if ( nrvals == 1 || statupscl_ == Stats::TakeNearest )
	return vals[0];

    Stats::CalcSetup rcsu( true );
    rcsu.require( stattype_ );
    Stats::RunCalc<float> rc( rcsu );
    for ( int idx=0; idx<vals.size(); idx++ )
    {
	if ( !mIsUdf(vals[idx]) )
	    rc.addValue( vals[idx], wts[idx] );
    }

    return (float)rc.getValue( stattype_ );
}


static bool allLithAreUndef( const Strat::LayerSequence& seq )
{
    for ( int ilay=0; ilay<seq.size(); ilay++ )
    {
	if ( !seq.layers()[ilay]->lithology().isUdf() )
	    return false;
    }

    return true;
}


float Strat::LaySeqAttribCalc::getGlobalValue( const LayerSequence& seq ) const
{
    if ( validx_ < 0 ) return mUdf(float);

    ObjectSet<const Strat::Layer> layers;
    const bool isallundef = allLithAreUndef( seq );
    for ( int ilay=0; ilay<seq.size(); ilay++ )
    {
	const Strat::Layer* lay = seq.layers()[ilay];
	for ( int iun=0; iun<units_.size(); iun++ )
	{
	    if ( units_[iun]->isParentOf( lay->unitRef() ) &&
		 ( isallundef || liths_.isPresent(&lay->lithology())) )
		layers += lay;
	}
    }
    if ( layers.isEmpty() )
	return stattype_ == Stats::Count ? 0 : mUdf(float);

    TypeSet<float> vals; TypeSet<float> wts;
    const bool isthick = &attr_.prop_ == &Strat::Layer::thicknessRef();
    for ( int ilay=0; ilay<layers.size(); ilay++ )
    {
	const Strat::Layer& lay = *layers[ilay];
	const float val = lay.value( validx_ );
	if ( !mIsUdf(val) )
	{
	    vals += val;
	    if ( !isthick )
		wts += lay.thickness();
	}
    }

    if ( vals.isEmpty() )
	return mUdf(float);
    applyTransform( vals );
    if ( vals.size() == 1 )
	return vals[0];

    Stats::CalcSetup rcsu( !isthick );
    rcsu.require( stattype_ );
    Stats::RunCalc<float> rc( rcsu );
    for ( int idx=0; idx<vals.size(); idx++ )
    {
	if ( !mIsUdf(vals[idx]) )
	    rc.addValue( vals[idx], isthick ? 1 : wts[idx] );
    }

    return (float)rc.getValue( stattype_ );
}


void Strat::LaySeqAttribCalc::applyTransform( TypeSet<float>& vals ) const
{
    if ( !attr_.hasTransform() )
	return;

    float tval = attr_.transformval_;
    if ( tval != LaySeqAttrib::Pow )
	tval = Math::Log( tval );

    for ( int idx=0; idx<vals.size(); idx++ )
    {
	float& val = vals[idx];
	switch ( attr_.transform_ )
	{
	    case LaySeqAttrib::Pow:
		val = Math::PowerOf( val, tval );
	    break;
	    case LaySeqAttrib::Log:
		val = Math::Log( val );
		if ( !mIsUdf(val) && !mIsUdf(tval) )
		    val /= tval;
	    break;
	    case LaySeqAttrib::Exp:
		val = mIsUdf(tval) ? mUdf(float) : Math::Exp( val*tval );
	    break;
	}
    }
}


Strat::LayModAttribCalc::LayModAttribCalc( const Strat::LayerModel& lm,
					   const Strat::LaySeqAttribSet& lsas,
					   DataPointSet& res )
    : Executor("Attribute extraction")
    , lm_(lm)
    , dps_(res)
    , seqidx_(0)
    , msg_("Extracting layer attributes")
    , calczwdth_(SI().zRange(false).step / 2)
{
    if ( SI().zIsTime() )
	calczwdth_ *= 2000;

    for ( int idx=0; idx<lsas.size(); idx++ )
    {
	const LaySeqAttrib& lsa = *lsas[idx];
	const int dpsidx = dps_.indexOf( lsa.name() );
	if ( dpsidx < 0 )
	    continue;

	LaySeqAttribCalc* calc = new LaySeqAttribCalc( lsa, lm );
	calcs_ += calc;
	dpscidxs_ += dpsidx;
    }
}


Strat::LayModAttribCalc::~LayModAttribCalc()
{
    deepErase( calcs_ );
}


#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }


int Strat::LayModAttribCalc::nextStep()
{
    const int dpssz = dps_.size();
    if ( dpssz < 1 )
	mErrRet("No data points for extraction")
    if ( seqidx_ >= lm_.size() )
	return Finished();

    const LayerSequence& seq = lm_.sequence( mCast(int,seqidx_) );
    DataPointSet::RowID dpsrid = 0;
    while ( dpsrid < dpssz && dps_.trcNr(dpsrid) != seqidx_ + 1 )
	dpsrid++;

    while ( dpsrid < dpssz && dps_.trcNr(dpsrid) == seqidx_ + 1 )
    {
	DataPointSet::DataRow dr( dps_.dataRow(dpsrid) );
	float* dpsvals = dps_.getValues( dpsrid );
	const float z = dps_.z( dpsrid );
	const Interval<float> zrg( z-calczwdth_, z+calczwdth_ );
	for ( int idx=0; idx<dpscidxs_.size(); idx++ )
	{
	    const float val = calcs_[idx]->getValue( seq, zrg );
	    dpsvals[ dpscidxs_[idx] ] = val;
	}

	dpsrid++;
    }

    seqidx_++;
    return seqidx_ >= lm_.size() ? Finished() : MoreToDo();
}
