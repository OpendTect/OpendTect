/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayseqattrib.h"

#include "ascstream.h"
#include "datapointset.h"
#include "iopar.h"
#include "keystrs.h"
#include "propertyref.h"
#include "separstr.h"
#include "statruncalc.h"
#include "strattransl.h"
#include "stratlayer.h"
#include "stratlayersequence.h"
#include "stratlayermodel.h"
#include "stratlayseqattribcalc.h"
#include "stratreftree.h"
#include "survinfo.h"

#include <math.h>


#define mFileType "Layer Sequence Attribute Set"
static const char* sKeyFileType = mFileType;
mDefSimpleTranslators(StratLayerSequenceAttribSet,mFileType,od,Attr);
mDefineEnumUtils(Strat::LaySeqAttrib,Transform,"Value Transformation")
{ "Power", "Log", "Exp", nullptr };


Strat::LaySeqAttrib* Strat::LaySeqAttribSet::gtAttr( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( attr(idx).name() == nm )
	    return const_cast<Strat::LaySeqAttrib*>( (*this)[idx] );
    }
    return nullptr;
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
    erase();

    for ( int idx=0; ; idx++ )
    {
	const BufferString res =
			iop.find( IOPar::compKey(sKey::Property(),idx) );
	if ( res.isEmpty() )
	    break;

	const PropertyRef* pr = PROPS().getByName( res, false );
	if ( !pr && Strat::Layer::thicknessRef().name() == res )
	    pr = &Strat::Layer::thicknessRef();

	if ( !pr )
	    continue;

	BufferString nm; mDoIOPar( get, sKey::Name(), nm );
	if ( nm.isEmpty() || attr(nm.buf()) )
	    continue;

	auto* lsa = new LaySeqAttrib( *this, *pr, nm );
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
	    lsa->transformval_ = fms.getFValue( 1 );
	    if ( lsa->transform_==LaySeqAttrib::Log && lsa->transformval_<0 )
		mSetUdf( lsa->transformval_ );
	}
	*this += lsa;
    }
}


bool Strat::LaySeqAttribSet::getFrom( od_istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	return false;

    IOPar iop; iop.getFrom( astrm );
    erase(); getFrom( iop );
    return true;
}


bool Strat::LaySeqAttribSet::putTo( od_ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	return false;

    IOPar iop; putTo( iop );
    iop.putTo( astrm );
    return strm.isOK();
}


Strat::LaySeqAttribCalc::LaySeqAttribCalc( const Strat::LaySeqAttrib& desc,
				   const Strat::LayerModel& lm )
    : attr_(desc)
    , validx_(-1)
{
    if ( attr_.islocal_ )
    {
	statupscl_ = (Stats::UpscaleType)Stats::UpscaleTypeDef()
					.convert( desc.stat_.buf() );
	stattype_ = Stats::typeFor( statupscl_ );
    }
    else
    {
	stattype_ = (Stats::Type)Stats::TypeDef().convert( desc.stat_.buf() );
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


bool Strat::LaySeqAttribCalc::isDist() const
{
    return attr_.prop_.stdType() == Mnemonic::Dist;
}


bool Strat::LaySeqAttribCalc::isVel() const
{
    return attr_.prop_.stdType() == Mnemonic::Vel;
}


float Strat::LaySeqAttribCalc::getValue( const LayerSequence& seq,
					 const Interval<float>& zpos ) const
{
    return attr_.islocal_ ? getLocalValue( seq, zpos ) : getGlobalValue(seq);
}


#define mRetUdfVal \
	return isDist() ? 0 : mUdf(float)


float Strat::LaySeqAttribCalc::getLocalValue( const LayerSequence& seq,
					  const Interval<float>& zrg ) const
{
    if ( validx_ < 0 || seq.isEmpty() )
	mRetUdfVal;

    const ObjectSet<Layer>& lays = seq.layers();
    if ( statupscl_ == Stats::TakeNearest || zrg.stop < zrg.start )
    {
	const float depth = zrg.center();
	const int ilay = seq.layerIdxAtZ( depth );
	if ( !lays.validIdx(ilay) )
	    mRetUdfVal;

	const float propval = lays[ilay]->value( validx_ );
	return propval;
    }

    auto* newseq = new LayerSequence( &seq.propertyRefs() );
    seq.getSequencePart( zrg, true, *newseq );
    if ( !newseq || newseq->isEmpty() )
	mRetUdfVal;

    const bool propisvel = isVel();
    TypeSet<float> vals; TypeSet<float> wts;
    for ( int ilay=0; ilay < newseq->size(); ilay++ )
    {
	const Strat::Layer* lay = newseq->layers()[ilay];
	if ( !lay )
	    continue;

	const float thickness = lay->thickness();
	const float propval = lay->value( validx_ );
	if ( mIsUdf(propval) || mIsUdf(thickness) || thickness < 1e-5f ||
		(propisvel && propval < 1e-5f) )
	    continue;

	wts += thickness;
	vals += propisvel ? 1.f / propval : propval;
    }

    if ( vals.isEmpty() )
	mRetUdfVal;

    applyTransform( vals );
    Stats::CalcSetup rcsu( true );
    rcsu.require( stattype_ );
    Stats::RunCalc<float> rc( rcsu );
    for ( int idx=0; idx<vals.size(); idx++ )
	rc.addValue( vals[idx], wts[idx] );

    const float rcout = mCast( float, rc.getValue( stattype_ ) );
    return propisvel ? 1.f / rcout : rcout;
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


#undef mRetUdfVal
#define mRetUdfVal \
	return isDist() || stattype_ == Stats::Count ? 0 : mUdf(float)


float Strat::LaySeqAttribCalc::getGlobalValue( const LayerSequence& seq ) const
{
    if ( validx_ < 0 || seq.isEmpty() )
	mRetUdfVal;

    const ObjectSet<Layer>& lays = seq.layers();
    const int nrlays = seq.size();

    ObjectSet<const Strat::Layer> layers;
    const bool isallundef = allLithAreUndef( seq );
    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	const Strat::Layer* lay = lays[ilay];
	for ( int iun=0; iun<units_.size(); iun++ )
	{
	    if ( units_[iun]->isParentOf( lay->unitRef() ) &&
		 ( isallundef || liths_.isPresent(&lay->lithology())) )
		layers += lay;
	}
    }
    if ( layers.isEmpty() )
	mRetUdfVal;

    TypeSet<float> vals; TypeSet<float> wts;
    const bool isthick = attr_.prop_.isThickness();
    const bool propisvel = isVel();
    for ( int ilay=0; ilay<layers.size(); ilay++ )
    {
	const Strat::Layer* lay = layers[ilay];
	if ( !lay )
	    continue;

	const float thickness = lay->thickness();
	const float propval =  isthick ? thickness
				       : lay->value( validx_ );

	if ( mIsUdf(propval) || mIsUdf(thickness) || thickness < 1e-5f ||
	     (propisvel && propval < 1e-5f) )
	    continue;

	wts += thickness;
	vals += propisvel ? 1.f / propval : propval;
    }

    if ( vals.isEmpty() )
	mRetUdfVal;

    applyTransform( vals );
    Stats::CalcSetup rcsu( !isthick );
    rcsu.require( stattype_ );
    Stats::RunCalc<float> rc( rcsu );
    for ( int idx=0; idx<vals.size(); idx++ )
	rc.addValue( vals[idx], isthick ? 1.f : wts[idx] );

    const float rcout = mCast( float, rc.getValue( stattype_ ) );
    return propisvel ? 1.f / rcout : rcout;
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
    , msg_(tr("Extracting layer attributes"))
    , stoplvl_(0)
{
    for ( int idx=0; idx<lsas.size(); idx++ )
    {
	const LaySeqAttrib& lsa = *lsas[idx];
	const int dpsidx = dps_.indexOf( lsa.name() );
	if ( dpsidx < 0 )
	    continue;

	calcs_ += new LaySeqAttribCalc( lsa, lm );
	dpscidxs_ += dpsidx;
    }
}


Strat::LayModAttribCalc::~LayModAttribCalc()
{
    deepErase( calcs_ );
    deepErase( extrgates_ );
}


#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }


od_int64 Strat::LayModAttribCalc::totalNr() const
{ return lm_.size(); }


int Strat::LayModAttribCalc::nextStep()
{
    const int dpssz = dps_.size();
    if ( dpssz < 1 )
	mErrRet(tr("No data points for extraction"))
    if ( seqidx_ >= lm_.size() )
	return Finished();

    const int dpsdepthidx = dps_.indexOf( sKey::Depth() );
    if ( dpsdepthidx<0 )
	mErrRet( tr("No depth data found") )
    const LayerSequence& seq = lm_.sequence( mCast(int,seqidx_) );
    uiString errmsg = tr("No extraction interval specified "
                         "for pseudo-well number %1");
    DataPointSet::RowID dpsrid = 0;
    while ( dpsrid < dpssz && dps_.trcNr(dpsrid) != seqidx_ + 1 )
	dpsrid++;

    const int dpthidx = dps_.indexOf( sKey::Depth() );
    if ( dpthidx < 0 )
	mErrRet(tr("No 'Depth' column in input data"))

    const bool zinft = SI().depthsInFeet();
    int pointidx = 0;
    const float stoplvldpth = stoplvl_ ? seq.depthPositionOf( *stoplvl_ )
					: mUdf(float);

    while ( dpsrid < dpssz && dps_.trcNr(dpsrid) == seqidx_ + 1 )
    {
	DataPointSet::DataRow dr( dps_.dataRow(dpsrid) );
	float* dpsvals = dps_.getValues( dpsrid );
	float z = dpsvals[dpthidx];
	if ( zinft )
	   z *= mFromFeetFactorF;

	Interval<float> zrg;
	const int seqnb = mCast( int, seqidx_ );
	if ( extrgates_.isEmpty() )
	{
	    const int ilay = seq.nearestLayerIdxAtZ( z );
	    const float halfwdth = seq.layers()[ilay]->thickness() / 2.f;
	    zrg.setFrom( Interval<float>(z-halfwdth, z+halfwdth) );
	}
	else
	{
	    if ( !extrgates_.validIdx(seqnb) )
	    {
		errmsg.arg( seqnb+1 );
		mErrRet( errmsg )
	    }

	    const ExtrGateSet& gateset( *extrgates_[seqnb] );
	    if ( !gateset.validIdx(pointidx) )
	    {
		errmsg.arg( seqnb+1 );
		mErrRet( errmsg )
	    }

	    zrg.setFrom( gateset[pointidx] );
	}

	bool paststop = false;
	if ( !mIsUdf(stoplvldpth) )
	{
	    if ( zrg.start > stoplvldpth )
		paststop = true;
	    else if ( zrg.stop > stoplvldpth )
		zrg.stop = stoplvldpth;
	}

	if ( paststop )
	    dps_.setInactive( dpsrid, true );
	else
	{
	    for ( int idx=0; idx<dpscidxs_.size(); idx++ )
	    {
		const float val = calcs_[idx]->getValue( seq, zrg );
		dpsvals[ dpscidxs_[idx] ] = val;
	    }
	}

	dpsrid++;
	pointidx++;
    }

    seqidx_++;
    return seqidx_ >= lm_.size() ? doFinish() : MoreToDo();
}


void Strat::LayModAttribCalc::setExtrGates(
	const ObjectSet<Strat::LayModAttribCalc::ExtrGateSet>& extrgts,
	const Strat::Level* stoplvl )
{
    stoplvl_ = stoplvl;
    deepErase( extrgates_ );
    deepCopy( extrgates_, extrgts );
}


int Strat::LayModAttribCalc::doFinish()
{
    dps_.purgeInactive();
    return Finished();
}
