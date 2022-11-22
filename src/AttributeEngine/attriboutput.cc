/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attriboutput.h"

#include "attribdataholder.h"
#include "convmemvalseries.h"
#include "ioman.h"
#include "posinfo2d.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seistype.h"
#include "seiswrite.h"
#include "separstr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace Attrib
{

const char* Output::outputstr()	    { return sKey::Output(); }
const char* Output::cubekey()	    { return sKey::Cube(); }
const char* Output::surfkey()	    { return sKey::Surface(); }
const char* Output::tskey()	    { return "Trace Selection"; }
const char* Output::scalekey()	    { return sKey::Scale(); }
const char* Output::varzlinekey()   { return "Variable Z Line"; }

const char* SeisTrcStorOutput::seisidkey()	{ return "Seismic.ID"; }
const char* SeisTrcStorOutput::attribkey()	{ return sKey::Attributes(); }
const char* SeisTrcStorOutput::inlrangekey()	{ return "In-line range"; }
const char* SeisTrcStorOutput::crlrangekey()	{ return "Cross-line range"; }
const char* SeisTrcStorOutput::depthrangekey()	{ return "Depth range"; }

const char* LocationOutput::filenamekey()	{ return "Output.File name"; }
const char* LocationOutput::locationkey()	{ return "Locations"; }
const char* LocationOutput::attribkey()		{ return sKey::Attribute(); }
const char* LocationOutput::surfidkey()		{ return "Surface.ID"; }


Output::Output()
    : seldata_(new Seis::RangeSelData(true))
{
    seldata_->setIsAll( true );
}


Output::~Output()
{
    delete seldata_;
}


bool Output::wantsOutput( const BinID& bid ) const
{
    return wantsOutput( SI().transform( bid ) );
}


bool Output::wantsOutput( const Coord& c ) const
{
    return wantsOutput( SI().transform( c ) );
}


TypeSet<Interval<int> > Output::getLocalZRanges( const BinID& bid, float f,
						 TypeSet<float>& ts ) const
{
    return getLocalZRanges( SI().transform(bid), f, ts );
}


TypeSet<Interval<int> > Output::getLocalZRanges( const Coord& c, float f,
						 TypeSet<float>& ts ) const
{
    return getLocalZRanges( SI().transform(c), f, ts );
}


void Output::ensureSelType( Seis::SelType st )
{
    if ( seldata_->type() != st )
    {
	Seis::SelData* newseldata = Seis::SelData::get( st );
	newseldata->setGeomID( seldata_->geomID() );
	delete seldata_; seldata_ = newseldata;
    }
}


void Output::doSetGeometry( const TrcKeyZSampling& cs )
{
    if ( cs.isEmpty() ) return;

    ensureSelType( Seis::Range );
    ((Seis::RangeSelData*)seldata_)->cubeSampling() = cs;
}


Pos::GeomID Output::curGeomID() const
{ return seldata_->geomID(); }



// DataPackOutput
DataPackOutput::DataPackOutput( const TrcKeyZSampling& cs )
    : desiredvolume_(cs)
    , dcsampling_(cs)
    , output_(0)
    , udfval_(mUdf(float))
{
}


DataPackOutput::~DataPackOutput()
{}


bool DataPackOutput::getDesiredVolume( TrcKeyZSampling& cs ) const
{ cs=desiredvolume_; return true; }


bool DataPackOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume_.hsamp_.includes(bid); }


TypeSet<Interval<int> > DataPackOutput::getLocalZRanges( const BinID&,
							 float zstep,
							 TypeSet<float>& ) const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT32( desiredvolume_.zsamp_.start / zstep ),
				mNINT32( desiredvolume_.zsamp_.stop / zstep ) );
	const_cast<DataPackOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


void DataPackOutput::adjustInlCrlStep( const TrcKeyZSampling& possvol )
{
    desiredvolume_.adjustTo( possvol );
    dcsampling_ = desiredvolume_;
}


void DataPackOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    if ( !output_ )
    {
	BinDataDesc bdd( false, true, sizeof(float) );
	if ( data.nrSeries() > 0 )
	{
	    mDynamicCastGet(ConvMemValueSeries<float>*,cmvs,data.series(0));
	    if ( cmvs && cmvs->handlesUndef() )
		bdd = cmvs->dataDesc();
	}

	init( refstep, &bdd );
    }

    const TrcKeyZSampling& tkzs = output_->sampling();
    if ( !tkzs.hsamp_.includes(info.trcKey()) )
	return;

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
	if ( desout >= output_->nrComponents() )
	    output_->addComponent( sKey::EmptyString() );

    //something went wrong during memory allocation
    if ( output_->nrComponents() < desoutputs_.size() )
	return;

    // We have a problem.
    // If the integer Z indexes do not seem to be corresponding with requested
    // Z sampling positions, then we may have:
    // (1) A genuine shifted output request
    // (2) Input data where 0 is not on a sample
    // But ...
    // I think (1) is never the case because datapack output is requested in a
    // 'nice' way (i.e. not with 'random' shifts). Thus ... will assume that.
    // This tests pretty nicely

    const Interval<int> inputrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    const float z0 = tkzs.zsamp_.start / tkzs.zsamp_.step;
    const int outz0samp = mNINT32( z0 );
    const Interval<int> outrg( outz0samp, outz0samp+tkzs.zsamp_.nrSteps() );
    if ( !inputrg.overlaps(outrg,false) )
	return;

    const Interval<int> transrg( mMAX(inputrg.start, outrg.start),
				 mMIN(inputrg.stop, outrg.stop ) );
    const int lineidx = tkzs.hsamp_.lineRange().nearestIndex( info.inl());
    const int trcidx = tkzs.hsamp_.trcRange().nearestIndex( info.crl() );

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	const ValueSeries<float>* vals = data.series( desoutputs_[desout] );
	if ( !vals )
	    continue;

	Array3D<float>& outarr3d = output_->data( desout );
	const int zarrsz = outarr3d.info().getSize( 2 );

	for ( int idx=transrg.start; idx<=transrg.stop; idx++)
	{
	    const float val = vals->value( idx-data.z0_ );

	    int zoutidx = idx - outrg.start;
	    if ( zoutidx >= 0 && zoutidx < zarrsz )
		outarr3d.set( lineidx, trcidx, zoutidx, val );
	}
    }
}


const RegularSeisDataPack* DataPackOutput::getDataPack() const
{ return output_; }


RegularSeisDataPack* DataPackOutput::getDataPack( float refstep )
{
    if ( !output_ )
	init( refstep );

    return output_;
}


void DataPackOutput::init( float refstep, const BinDataDesc* bdd )
{
    output_ = new RegularSeisDataPack( sKey::EmptyString(), bdd );
    output_->setSampling( dcsampling_ );
    const_cast<StepInterval<float>& >(output_->sampling().zsamp_).step=refstep;
}



// SeisTrcStorOutput
SeisTrcStorOutput::SeisTrcStorOutput( const TrcKeyZSampling& cs,
				      const Pos::GeomID geomid )
    : desiredvolume_(cs)
    , auxpars_(0)
    , storid_(*new MultiID)
    , writer_(0)
    , trc_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , errmsg_(uiString::emptyString())
    , scaler_(0)
    , growtrctosi_(false)
{
    seldata_->setGeomID( geomid );
}


SeisTrcStorOutput::~SeisTrcStorOutput()
{
    delete writer_;
    delete &storid_;
    delete auxpars_;
    delete scaler_;
}


bool SeisTrcStorOutput::getDesiredVolume( TrcKeyZSampling& cs ) const
{
    cs = desiredvolume_;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredvolume_.hsamp_.includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const MultiID& storid )
{
    if ( !storid.isUdf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid );
	if ( !ioseisout )
	{
	    errmsg_ = tr("Cannot find seismic data with ID: %1").arg( storid );
	    return false;
	}
    }

    storid_ = storid;
    return true;
}


static bool isDataType( const char* reqtp )
{
    BufferString reqdatatype = reqtp;
    if ( reqdatatype == sKey::Steering() )
	reqdatatype = "Dip";

    const BufferStringSet alldatatypes( Seis::dataTypeNames() );
    return alldatatypes.isPresent( reqdatatype.buf() );
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars )
{
    errmsg_ = uiString::emptyString();
    PtrMan<IOPar> outppar = pars.subselect( IOPar::compKey(sKey::Output(),0) );
    if ( !outppar )
	outppar = pars.subselect( IOPar::compKey(sKey::Output(),1) );

    if ( !outppar )
    {
	errmsg_ = tr("Could not find Output keyword in parameter file");
	return false;
    }

    MultiID stormid;
    outppar->get( seisidkey(), stormid );
    if ( !setStorageID(stormid) )
    {
	errmsg_ = tr("Could not find output ID: %1").arg( stormid.toString() );
        return false;
    }

    SeparString sepstr( stormid.toString(), '|' );

    if ( sepstr[1] && *sepstr[1] )
	attribname_ = sepstr[1];

    if ( sepstr[2] && *sepstr[2] && isDataType(sepstr[2]) )
	datatype_ += sepstr[2];

    outppar->get( sKey::Names(), outpnames_ );
    const BufferString res = outppar->find( scalekey() );
    if ( !res.isEmpty() )
    {
	scaler_ = Scaler::get( res );
	if ( scaler_ && scaler_->isEmpty() )
	    { delete scaler_; scaler_ = 0; }
    }

    auxpars_ = pars.subselect("Aux");
    return doInit();
}//warning, only a small part of the old taken, see if some more is required


bool SeisTrcStorOutput::doInit()
{
    ensureSelType( Seis::Range );

    if ( !storid_.isUdf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = tr("Cannot find seismic data with ID: %1").arg( storid_ );
	    return false;
	}

	writer_ = new SeisTrcWriter( *ioseisout );
	is2d_ = writer_->is2D();

	if ( is2d_ && !datatype_.isEmpty() )
	    writer_->setDataType( datatype_.buf() );

	if ( auxpars_ )
	{
	    writer_->auxPars().merge( *auxpars_ );
	    deleteAndZeroPtr( auxpars_ );
	}
    }

    desiredvolume_.normalize();
    if ( !is2d_ )
    {
	TrcKeyZSampling& cs = ((Seis::RangeSelData*)seldata_)->cubeSampling();
	desiredvolume_.limitTo( cs );
    }

    return true;
}



class COLineKeyProvider : public GeomIDProvider
{
public:

    COLineKeyProvider( Pos::GeomID geomid )
    : geomid_( geomid ) {}

Pos::GeomID geomID() const override
{ return geomid_; }

Pos::GeomID geomid_;

};


void SeisTrcStorOutput::collectData( const DataHolder& data, float refstep,
				     const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size())
	return;

    const int sz = data.nrsamples_;
    DataCharacteristics dc;

    if ( !trc_ )
    {
	trc_ = new SeisTrc( sz, dc );
	trc_->info() = info;
	trc_->info().sampling.step = refstep;
	trc_->info().sampling.start = data.z0_*refstep;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }
    else if ( trc_->info().trcKey() != info.trcKey() )
    {
	errmsg_ = tr("merge components of two different traces!");
	return;
    }
    else
    {
	const int curnrcomps = trc_->nrComponents();
	for ( int idx=curnrcomps; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.series(desoutputs_[comp])->value(idx);
	    trc_->set( idx, val, comp );
	}
    }

    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = (float) scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }

    if ( !mIsEqual(desiredvolume_.zsamp_.step,trc_->info().sampling.step,1e-6) )
    {
	StepInterval<float> reqzrg = desiredvolume_.zsamp_;
	reqzrg.limitTo( trc_->zRange() );
	const int nrsamps = mCast( int, reqzrg.nrfSteps() + 1 );
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    SeisTrc temptrc( *trc_ );
	    trc_->info().sampling.step = desiredvolume_.zsamp_.step;
	    trc_->data().getComponent(icomp)->reSize( nrsamps );
	    for ( int isamp=0; isamp<nrsamps; isamp++ )
	    {
		float t = reqzrg.start + isamp * reqzrg.step;
		trc_->set( isamp, temptrc.getValue(t,icomp), icomp );
	    }
	}
    }
}


bool SeisTrcStorOutput::writeTrc()
{
    if ( !writer_ || !trc_ )
	return true;

    SeisTrc* usetrc = trc_;
    PtrMan<SeisTrc> tmptrc = nullptr;
    if ( growtrctosi_ )
    {
	tmptrc = trc_->getExtendedTo( SI().zRange(true), true );
	usetrc = tmptrc;
    }

    if ( !storinited_ )
    {
	SeisTrcTranslator* transl = 0;
	if ( writer_->is2D() )
	    writer_->setGeomIDProvider( new COLineKeyProvider(curGeomID()) );
	else
	{
	    transl = writer_->seisTranslator();
	    if ( transl )
		transl->setComponentNames( outpnames_ );
	}

	if ( !writer_->prepareWork(*usetrc) )
	    { errmsg_ = writer_->errMsg(); return false; }

	if ( writer_->is2D() )
	{
	    if ( writer_->linePutter() )
	    {
		mDynamicCastGet( SeisCBVS2DLinePutter*, lp,
				 writer_->linePutter() )
		if ( lp && lp->tr_ )
		    lp->tr_->setComponentNames( outpnames_ );
	    }
	}

	if ( transl )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& cis
		             = transl->componentInfo();
	    for ( int idx=0; idx<cis.size(); idx++ )
		cis[idx]->datatype = outptypes_.size() ? outptypes_[idx] :
							Seis::UnknowData;
	}

	storinited_ = true;
    }

    if ( !writer_->put(*usetrc) )
	{ errmsg_ = writer_->errMsg(); return false; }

    delete trc_;
    trc_ = 0;
    return true;
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRanges(
						    const BinID& bid,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT32(desiredvolume_.zsamp_.start/zstep),
				mNINT32(desiredvolume_.zsamp_.stop/zstep) );
	const_cast<SeisTrcStorOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


void SeisTrcStorOutput::deleteTrc()
{
    if ( trc_ ) delete trc_;
    trc_ = 0;
}


bool SeisTrcStorOutput::finishWrite()
{
    if ( !writer_->close() )
    {
	errmsg_ = writer_->errMsg();
	return false;
    }
    return true;
}



// TwoDOutput
TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			Pos::GeomID geomid)
    : errmsg_(0)
    , output_( 0 )
{
    seldata_->setGeomID( geomid );
    setGeometry( trg, zrg );
    const bool undeftrg = trg.start<=0 && Values::isUdf(trg.stop);
    seldata_->setIsAll( undeftrg );
}


TwoDOutput::~TwoDOutput()
{
    if ( output_ ) output_->unRef();
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_->crlRange().includes(bid.crl(),true);
}


void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    ensureSelType( Seis::Range );
    seldata_->setZRange( zrg );
    seldata_->setCrlRange( trg );
}


bool TwoDOutput::getDesiredVolume( TrcKeyZSampling& tkzs ) const
{
    tkzs.hsamp_.init( seldata_->geomID() );
    tkzs.hsamp_.setTrcRange( seldata_->crlRange() );
    const Interval<float> zrg( seldata_->zRange() );
    tkzs.zsamp_ = StepInterval<float>( zrg.start, zrg.stop, SI().zStep() );
    return true;
}


bool TwoDOutput::doInit()
{
    const Interval<int> rg( seldata_->crlRange() );
    if ( rg.start <= 0 && Values::isUdf(rg.stop) )
	seldata_->setIsAll( true );

    return true;
}


void TwoDOutput::collectData( const DataHolder& data, float refstep,
			      const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size() )
	return;

    if ( !output_ ) return;

    output_->dataset_ += data.clone();

    SeisTrcInfo* trcinfo = new SeisTrcInfo(info);
    trcinfo->sampling.step = refstep;
    //trcinfo->sampling.start = 0;
    output_->trcinfoset_ += trcinfo;
}


void TwoDOutput::setOutput( Data2DHolder& no )
{
    if ( output_ ) output_->unRef();
    output_ = &no;
    output_->ref();
}


TypeSet< Interval<int> > TwoDOutput::getLocalZRanges( const BinID& bid,
						      float zstep,
						      TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<float> zrg( seldata_->zRange() );
	Interval<int> interval( mNINT32(zrg.start/zstep),
				mNINT32(zrg.stop/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}



// LocationOutput
LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    ensureSelType( Seis::Table );
    seldata_->setIsAll( false );
    ((Seis::TableSelData*)seldata_)->binidValueSet().allowDuplicateBinIDs(true);
    ((Seis::TableSelData*)seldata_)->binidValueSet() = bidvalset;

    arebiddupl_ = areBIDDuplicated();
}


LocationOutput::~LocationOutput()
{}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    const BinID bid = SI().transform(info.coord);
    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    if ( !pos.isValid() ) return;

    const int desnrvals = desoutputs_.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    if ( !arebiddupl_ )
    {
	float* vals = bidvalset_.getVals( pos );
	computeAndSetVals( data, refstep, vals );
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	int lowz;
	DataHolder::getExtraZAndSampIdxFromExactZ( vals[0], refstep, lowz );
	const int highz = lowz + 1;
	bool isfulldataok = datarg.includes(lowz-1,false) &&
			    datarg.includes(highz+1,false);
	bool canusepartdata = data.nrsamples_<4 && datarg.includes(lowz,false)
			      && datarg.includes(highz,false);
	if ( isfulldataok || canusepartdata )
	    computeAndSetVals( data, refstep, vals );

	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
	    break;
    }
}


void LocationOutput::computeAndSetVals( const DataHolder& data, float refstep,
					float* vals )
{
    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	int serieidx = desoutputs_[comp];
	vals[comp+1] = data.getValue( serieidx, vals[0], refstep );
    }
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    return pos.isValid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz) const
{
    //TODO not 100% optimized, case of picksets for instance->find better algo
    TypeSet< Interval<int> > sampleinterval;

    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    while ( pos.isValid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	int zidx;
	DataHolder::getExtraZAndSampIdxFromExactZ( vals[0], zstep, zidx );
	Interval<int> interval( zidx, zidx );
	if ( arebiddupl_ )
	{
	    interval.start = zidx - 1;
	    interval.stop =  zidx + 2;
	}
	bool intvadded = sampleinterval.addIfNew( interval );
	if ( intvadded )
	    exactz += vals[0];

	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
	    break;
    }

    return sampleinterval;
}


bool LocationOutput::areBIDDuplicated() const
{
    BinIDValueSet tmpset(bidvalset_);
    tmpset.allowDuplicateBinIDs(false);

    return tmpset.totalSize()<bidvalset_.totalSize();
}



// TrcSelectionOutput
TrcSelectionOutput::TrcSelectionOutput( const BinIDValueSet& bidvalset,
					float outval )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
    , outval_(outval)
    , stdstarttime_(mUdf(float))
    , stdtrcsz_(mUdf(float))
{
    delete seldata_;
    Seis::TableSelData& sd = *new Seis::TableSelData( bidvalset );
    seldata_ = &sd;
    sd.binidValueSet().allowDuplicateBinIDs( true );

    const int nrinterv = bidvalset.nrVals() / 2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	float val = bidvalset.valRange(idx).start;
	if ( val < zmin ) zmin = val;
	val = bidvalset.valRange(idx+1).stop;
	if ( val > zmax ) zmax = val;
    }

    if ( !mIsUdf(zmin) && !mIsUdf(-zmax) )
    {
	BinIDValueSet::SPos pos; bidvalset.next( pos );
	const BinID bid0( bidvalset.getBinID(pos) );
	sd.binidValueSet().add( bid0, zmin );
	sd.binidValueSet().add( bid0, zmax );

	stdtrcsz_ = zmax - zmin;
	stdstarttime_ = zmin;
    }
}


TrcSelectionOutput::~TrcSelectionOutput()
{
}


void TrcSelectionOutput::collectData( const DataHolder& data, float refstep,
				      const SeisTrcInfo& info )
{
    const int nrcomp = data.nrSeries();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs_.size() )
	return;

    const int trcsz = mNINT32(stdtrcsz_/refstep) + 1;
    const float trcstarttime = mNINT32(stdstarttime_/refstep) * refstep;
    const int startidx = data.z0_ - mNINT32(trcstarttime/refstep);
    const int index = outpbuf_->find( info.binID() );

    SeisTrc* trc;
    if ( index == -1 )
    {
	const DataCharacteristics dc;
	trc = new SeisTrc( trcsz, dc );
	for ( int idx=trc->data().nrComponents(); idx<desoutputs_.size(); idx++)
	    trc->data().addComponent( trcsz, dc, false );

	trc->info() = info;
	trc->info().sampling.start = trcstarttime;
	trc->info().sampling.step = refstep;
    }
    else
	trc = outpbuf_->get( index );

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc->set( idx, outval_, comp );
	    else
	    {
		const float val =
		    data.series(desoutputs_[comp])->value(idx-startidx);
		trc->set( idx, val, comp );
	    }
	}
    }

    if ( index == -1 )
	outpbuf_->add( trc );
}


bool TrcSelectionOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    return pos.isValid();
}


void TrcSelectionOutput::setOutput( SeisTrcBuf* outp_ )
{
    outpbuf_ = outp_;
    if ( outpbuf_ )
	outpbuf_->erase();
}


void TrcSelectionOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
    seldata_->setZRange( intv );
}


void TrcSelectionOutput::setGeomID( Pos::GeomID geomid )
{
    seldata_->setGeomID( geomid );
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>&	) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    BinID binid;
    TypeSet<float> values;
    bidvalset_.get( pos, binid, values );
    TypeSet< Interval<int> > sampleinterval;

    if ( values.isEmpty() && !mIsUdf(stdstarttime_) )
    {
	const float zmax = stdstarttime_ + stdtrcsz_;
	Interval<int> interval( mNINT32(stdstarttime_/zstep),
				mNINT32(zmax/zstep) );
	sampleinterval += interval;
	return sampleinterval;
    }

    for ( int idx=0; idx<values.size()/2; idx+=2 )
    {
	Interval<int> interval( mNINT32(values[idx]/zstep),
				mNINT32(values[idx+1]/zstep) );
	sampleinterval += interval;
    }

    return sampleinterval;
}


bool TrcSelectionOutput::getDesiredVolume( TrcKeyZSampling& cs ) const
{
    Interval<int> inlrg = bidvalset_.inlRange();
    Interval<int> crlrg = bidvalset_.crlRange();
    Interval<float> zrg =
	Interval<float>( stdstarttime_, stdstarttime_ + stdtrcsz_ );
    TrcKeyZSampling trcselsampling( false );
    trcselsampling.include ( BinID( inlrg.start, crlrg.start), zrg.start);
    trcselsampling.include ( BinID( inlrg.stop, crlrg.stop), zrg.stop);
    if ( !cs.includes( trcselsampling ) )
	cs = trcselsampling;
    return true;
}


const TrcKeyZSampling Trc2DVarZStorOutput::getCS()
{
    TrcKeyZSampling cs;
    cs.hsamp_.start_.inl() = 0; cs.hsamp_.stop_.inl() = mUdf(int);
    cs.hsamp_.start_.crl() = 1; cs.hsamp_.stop_.crl() = mUdf(int);
    return cs;
}



// Trc2DVarZStorOutput
Trc2DVarZStorOutput::Trc2DVarZStorOutput( Pos::GeomID geomid,
					  DataPointSet* poszvalues,
					  float outval )
    : SeisTrcStorOutput( getCS(), geomid )
    , poszvalues_(poszvalues)
    , outval_(outval)
{
    is2d_ = true;
    const int nrinterv = (poszvalues_->nrCols()+1)/2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	int z1colidx = idx==0 ? idx : idx+poszvalues_->nrFixedCols();
	int z2colidx = idx==0 ? poszvalues_->nrFixedCols() : z1colidx+1;
	float val = poszvalues_->bivSet().valRange( z1colidx ).start;
	if ( val < zmin ) zmin = val;
	val = poszvalues_->bivSet().valRange( z2colidx ).stop;
	if ( val > zmax ) zmax = val;
    }

    setGeometry( getCS() );
    seldata_->setZRange( Interval<float>(zmin,zmax) );
    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


Trc2DVarZStorOutput::~Trc2DVarZStorOutput()
{}


void Trc2DVarZStorOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
}


bool Trc2DVarZStorOutput::doInit()
{
    ensureSelType( Seis::Range );
    if ( !storid_.isUdf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = tr("Cannot find seismic data with ID: %1").arg( storid_ );
	    return false;
	}

	writer_ = new SeisTrcWriter( *ioseisout );
	if ( !writer_->is2D() )
	{
	    errmsg_ = tr( "Seismic data with ID: %1 is not 2D\n"
			 "Cannot create 2D output.").arg( storid_ );
	    return false;
	}

	if ( auxpars_ )
	{
	    writer_->auxPars().merge( *auxpars_ );
	    deleteAndZeroPtr( auxpars_ );
	}
    }

    desiredvolume_.normalize();
    return true;
}


void Trc2DVarZStorOutput::collectData( const DataHolder& data, float refstep,
				     const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size())
	return;

    const int trcsz = mNINT32(stdtrcsz_/refstep) + 1;
    const float trcstarttime = mNINT32(stdstarttime_/refstep) * refstep;
    const int startidx = data.z0_ - mNINT32(trcstarttime/refstep);
    DataCharacteristics dc;

    if ( !trc_ )
    {
	trc_ = new SeisTrc( trcsz, dc );
	trc_->info() = info;
	trc_->info().sampling.step = refstep;
	trc_->info().sampling.start = trcstarttime;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( trcsz, dc, false );
    }
    else if ( trc_->info().trcKey() != info.trcKey() )
    {
	errmsg_ = tr("merge components of two different traces!");
	return;
    }
    else
    {
	const int curnrcomps = trc_->nrComponents();
	for ( int idx=curnrcomps; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( trcsz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc_->set( idx, outval_, comp );
	    else
	    {
		float val = data.series(desoutputs_[comp])->value(idx-startidx);
		trc_->set(idx, val, comp);
	    }
	}
    }

    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcsz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = (float) scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }
}


TypeSet< Interval<int> > Trc2DVarZStorOutput::getLocalZRanges(
						    const Coord& coord,
						    float zstep,
						    TypeSet<float>& ) const
{
    TypeSet< Interval<int> > sampleinterval;
    DataPointSet::RowID rowid = poszvalues_->findFirst( coord );

    if ( rowid< 0 )
    {
	for ( int idx=0; idx<poszvalues_->size()-1; idx++ )
	{
	    if ( coord > poszvalues_->coord( idx )
		&& coord < poszvalues_->coord( idx+1 ) )
	    {
		const double distn = coord.distTo( poszvalues_->coord(idx) );
		const double distnp1 = coord.distTo(poszvalues_->coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 )
		    { rowid = idx; break; }
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 )
		    { rowid = idx+1; break; }
	    }
	}
    }

    if ( rowid< 0 ) return sampleinterval;

    const BinID bid = poszvalues_->binID(rowid);
    for ( int idx=rowid; idx<poszvalues_->size(); idx++ )
    {
	if ( poszvalues_->binID( idx ) != bid ) break;
	if ( mIsEqual( poszvalues_->coord(idx).x, coord.x, 1e-3 )
	   &&mIsEqual( poszvalues_->coord(idx).y, coord.y, 1e-3 ) )
	{
	    Interval<int> interval( mNINT32(poszvalues_->z(idx)/zstep),
				    mNINT32(poszvalues_->value(0,idx)/zstep) );
	    sampleinterval += interval;
	    const int nrextrazintv = (poszvalues_->nrCols()-1)/2;
	    for ( int idi=0; idi<nrextrazintv; idi+=2 ) //to keep it general
	    {
		Interval<int> intv(
			mNINT32(poszvalues_->value(idi+1,idx)/zstep),
			mNINT32(poszvalues_->value(idi+2,idx)/zstep));
		sampleinterval += intv;
	    }
	}
    }

    return sampleinterval;
}


bool Trc2DVarZStorOutput::wantsOutput( const Coord& coord ) const
{
    //TODO : for some reason horizon coords in 2D are now rounded, check why
    Coord roundedcoord( (int)coord.x, (int)coord.y );
    return poszvalues_->findFirst( roundedcoord ) > -1;
}


bool Trc2DVarZStorOutput::finishWrite()
{
    return writer_->close();
}



// TableOutput
TableOutput::TableOutput( DataPointSet& datapointset, int firstcol )
    : datapointset_(datapointset)
    , firstattrcol_(firstcol)
    , mediandisttrcs_(mUdf(float))
{
    ensureSelType( Seis::Table );
    seldata_->setIsAll( false );
    ((Seis::TableSelData*)seldata_)->binidValueSet().allowDuplicateBinIDs(true);
    ((Seis::TableSelData*)seldata_)->binidValueSet() = datapointset_.bivSet();

    arebiddupl_ = areBIDDuplicated();
}


TableOutput::~TableOutput()
{}


bool TableOutput::getDesiredVolume( TrcKeyZSampling& tkzs ) const
{
    return datapointset_.getRange( tkzs );
}


void TableOutput::setMedianDistBetwTrcs( float mediandist )
{
    mediandisttrcs_ = mediandist;
}


void TableOutput::collectData( const DataHolder& data, float refstep,
			       const SeisTrcInfo& info )
{
    const TrcKey tkey = info.trcKey();
    const BinID dpsbid = tkey.is2D() == datapointset_.is2D() ? tkey.position()
					: SI().transform( tkey.getCoord() );
    DataPointSet::RowID rid = datapointset_.findFirst( dpsbid );
    if ( rid < 0 )
	return;

    const int desnrvals = desoutputs_.size() + firstattrcol_;
    if ( datapointset_.nrCols() < desnrvals )
	datapointset_.bivSet().setNrVals(desnrvals+datapointset_.nrFixedCols());

    if ( !arebiddupl_ )
    {
	float* vals = datapointset_.getValues( rid );
	computeAndSetVals( data, refstep, datapointset_.z(rid), vals );
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    for ( int idx=rid; idx<datapointset_.size(); idx++ )
    {
	if ( datapointset_.binID(idx) != dpsbid )
	    break;

	const float zval = datapointset_.z(idx);
	float* vals = datapointset_.getValues( idx );
	int lowz;
	DataHolder::getExtraZAndSampIdxFromExactZ( zval, refstep, lowz );
	const int highz = lowz + 1;
	bool isfulldataok = datarg.includes(lowz-1,false) &&
			    datarg.includes(highz+1,false);
	bool canusepartdata = data.nrsamples_<4 && datarg.includes(lowz,false)
			      && datarg.includes(highz,false);
	if ( isfulldataok || canusepartdata )
	    computeAndSetVals( data, refstep, zval, vals );
    }
}


void TableOutput::computeAndSetVals( const DataHolder& data, float refstep,
				     float zval, float* vals )
{
    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	int serieidx = desoutputs_[comp];
	vals[comp+firstattrcol_] = data.getValue( serieidx, zval, refstep );
    }
}


bool TableOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::SPos pos = datapointset_.bivSet().find( bid );
    return pos.isValid();
}


bool TableOutput::wantsOutput( const Coord& coord ) const
{
    return datapointset_.findFirst( coord ) > -1;
}


bool TableOutput::wantsOutput( const TrcKey& tkey ) const
{
    return tkey.is2D() == datapointset_.is2D() ?
			datapointset_.findFirst( tkey ) > -1 :
			datapointset_.findFirst( tkey.getCoord() ) > -1;
}


bool TableOutput::useCoords() const
{
    return false;
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;

    const BinIDValueSet& bvs = datapointset_.bivSet();
    BinIDValueSet::SPos pos = bvs.find( bid );

    DataPointSet::RowID rid = datapointset_.getRowID( pos );
    while ( pos.isValid() && bid == bvs.getBinID(pos) )
    {
	addLocalInterval( sampleinterval, exactz, rid, zstep );
	datapointset_.bivSet().next( pos );
	rid++;
    }

    return sampleinterval;
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const Coord& coord, float zstep,
						TypeSet<float>& exactz) const
{
    TrcKey tkey;
    tkey.setIs2D( datapointset_.is2D() ).setFrom( coord );
    return getLocalZRanges( tkey, zstep, exactz );
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const TrcKey& tkey, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;
    if ( tkey.isUdf() )
	return sampleinterval;

    const BinID dpsbid = tkey.is2D() == datapointset_.is2D() ? tkey.position()
					: SI().transform( tkey.getCoord() );
    DataPointSet::RowID rid = datapointset_.findFirst( dpsbid );
    if ( rid < 0 )
	return sampleinterval;

    addLocalInterval( sampleinterval, exactz, rid, zstep );
    for ( int idx=rid+1; idx<datapointset_.size(); idx++ )
    {
	if ( datapointset_.binID(idx) != dpsbid )
	    break;

	addLocalInterval( sampleinterval, exactz, idx, zstep );
    }

    return sampleinterval;
}


void TableOutput::addLocalInterval( TypeSet< Interval<int> >& sampintv,
				    TypeSet<float>& exactz,
				    int rid, float zstep ) const
{
    const float zval = datapointset_.z(rid);
    int zidx;
    DataHolder::getExtraZAndSampIdxFromExactZ( zval, zstep, zidx );
    Interval<int> interval( zidx, zidx );

    //Necessary if bid are duplicated and for a chain of attribs with stepout
    interval.start = zidx - 1;
    interval.stop =  zidx + 2;

    bool intvadded = sampintv.addIfNew( interval );
    if ( intvadded )
	exactz += zval;
}


bool TableOutput::areBIDDuplicated() const
{
    if ( datapointset_.isEmpty() ) return false;

    BinID prevbid( datapointset_.binID(0) );
    for ( int idx=1; idx<datapointset_.size(); idx++ )
    {
	const BinID bid( datapointset_.binID(idx) );
	if ( bid == prevbid ) return true;
	prevbid = bid;
    }

    return false;
}

} // namespace Attrib
