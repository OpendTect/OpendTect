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
#include "keystrs.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seistype.h"
#include "seiswrite.h"
#include "separstr.h"
#include "survinfo.h"


namespace Attrib
{

const char* Output::outputstr()		{ return sKey::Output(); }
const char* Output::cubekey()		{ return sKey::Cube(); }
const char* Output::surfkey()		{ return sKey::Surface(); }
const char* Output::tskey()		{ return "Trace Selection"; }
const char* Output::scalekey()		{ return sKey::Scale(); }
const char* Output::varzlinekey()	{ return "Variable Z Line"; }

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
    if ( cs.isEmpty() )
	return;

    ensureSelType( Seis::Range );
    ((Seis::RangeSelData*)seldata_)->cubeSampling() = cs;
}


Pos::GeomID Output::curGeomID() const
{
    return seldata_->geomID();
}



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
{
    cs = desiredvolume_;
    return true;
}


bool DataPackOutput::useCoords() const
{
    return false;
}


bool DataPackOutput::wantsOutput( const Coord& crd ) const
{
    return desiredvolume_.hsamp_.includes( SI().transform(crd) );
}


bool DataPackOutput::wantsOutput( const TrcKey& tk ) const
{
    return desiredvolume_.hsamp_.includes( tk.binID() );
}


TypeSet<Interval<int> > DataPackOutput::getLocalZRanges( const TrcKey&,
							 float zstep,
							 TypeSet<float>& ) const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT32(desiredvolume_.zsamp_.start_ / zstep),
				mNINT32(desiredvolume_.zsamp_.stop_ / zstep) );
	const_cast<DataPackOutput*>(this)->sampleinterval_ += interval;
    }

    return sampleinterval_;
}


TypeSet<Interval<int> > DataPackOutput::getLocalZRanges( const Coord&,
							 float zstep,
							 TypeSet<float>& ) const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT32(desiredvolume_.zsamp_.start_ / zstep),
				mNINT32(desiredvolume_.zsamp_.stop_ / zstep) );
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
    const float z0 = tkzs.zsamp_.start_ / tkzs.zsamp_.step_;
    const int outz0samp = mNINT32( z0 );
    const Interval<int> outrg( outz0samp, outz0samp+tkzs.zsamp_.nrSteps() );
    if ( !inputrg.overlaps(outrg,false) )
	return;

    const Interval<int> transrg( mMAX(inputrg.start_, outrg.start_),
                                 mMIN(inputrg.stop_, outrg.stop_ ) );
    const int lineidx = tkzs.hsamp_.lineRange().nearestIndex( info.inl());
    const int trcidx = tkzs.hsamp_.trcRange().nearestIndex( info.crl() );

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	const ValueSeries<float>* vals = data.series( desoutputs_[desout] );
	if ( !vals )
	    continue;

	Array3D<float>& outarr3d = output_->data( desout );
	const int zarrsz = outarr3d.info().getSize( 2 );

        for ( int idx=transrg.start_; idx<=transrg.stop_; idx++)
	{
	    const float val = vals->value( idx-data.z0_ );

            int zoutidx = idx - outrg.start_;
	    if ( zoutidx >= 0 && zoutidx < zarrsz )
		outarr3d.set( lineidx, trcidx, zoutidx, val );
	}
    }
}


const RegularSeisDataPack* DataPackOutput::getDataPack() const
{
    return output_.ptr();
}


RegularSeisDataPack* DataPackOutput::getDataPack( float refstep )
{
    if ( !output_ )
	init( refstep );

    return output_.ptr();
}


void DataPackOutput::init( float refstep, const BinDataDesc* bdd )
{
    output_ = new RegularSeisDataPack( sKey::EmptyString(), bdd );
    output_->setSampling( dcsampling_ );
    const_cast<StepInterval<float>& >(output_->sampling().zsamp_).step_=refstep;
}



// SeisTrcStorOutput
SeisTrcStorOutput::SeisTrcStorOutput( const TrcKeyZSampling& cs,
				      const Pos::GeomID& geomid )
    : storid_(*new MultiID)
    , desiredvolume_(cs)
    , errmsg_(uiString::emptyString())
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


bool SeisTrcStorOutput::useCoords() const
{
    return false;
}


bool SeisTrcStorOutput::wantsOutput( const Coord& crd ) const
{
    return desiredvolume_.hsamp_.includes( SI().transform(crd) );
}


bool SeisTrcStorOutput::wantsOutput( const TrcKey& tk ) const
{
    return desiredvolume_.hsamp_.includes( tk.binID() );
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

    if ( !sepstr[1].isEmpty() )
	attribname_ = sepstr[1];

    if ( !sepstr[2].isEmpty() && isDataType(sepstr[2]) )
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
	    deleteAndNullPtr( auxpars_ );
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



class COGeomIDProvider : public Pos::GeomIDProvider
{
public:
COGeomIDProvider( const Pos::GeomID& geomid )
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
	trc_->info().sampling_.step_ = refstep;
	trc_->info().sampling_.start_ = data.z0_*refstep;
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

    if ( !mIsEqual(desiredvolume_.zsamp_.step_,
		   trc_->info().sampling_.step_,1e-6) )
    {
	StepInterval<float> reqzrg = desiredvolume_.zsamp_;
	reqzrg.limitTo( trc_->zRange() );
	const int nrsamps = mCast( int, reqzrg.nrfSteps() + 1 );
	SeisTrc temptrc( *trc_ );
	trc_->info().sampling_.step_ = desiredvolume_.zsamp_.step_;
	trc_->reSize( nrsamps, false );
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int isamp=0; isamp<nrsamps; isamp++ )
	    {
		const float t = reqzrg.start_ + isamp * reqzrg.step_;
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
	usetrc = tmptrc.ptr();
    }

    if ( !storinited_ )
    {
	SeisTrcTranslator* transl = 0;
	if ( writer_->is2D() )
	    writer_->setGeomIDProvider( new COGeomIDProvider(curGeomID()) );
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
		cis[idx]->datatype_ = outptypes_.size() ? outptypes_[idx] :
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
						    const TrcKey&,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
        Interval<int> interval( mNINT32(desiredvolume_.zsamp_.start_/zstep),
				mNINT32(desiredvolume_.zsamp_.stop_/zstep) );
	const_cast<SeisTrcStorOutput*>(this)->sampleinterval_ += interval;
    }

    return sampleinterval_;
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRanges(
						    const Coord&,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT32(desiredvolume_.zsamp_.start_/zstep),
				mNINT32(desiredvolume_.zsamp_.stop_/zstep) );
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
			const Pos::GeomID& geomid )
{
    seldata_->setGeomID( geomid );
    setGeometry( trg, zrg );
    const bool undeftrg = trg.start_<=0 && Values::isUdf(trg.stop_);
    seldata_->setIsAll( undeftrg );
}


TwoDOutput::~TwoDOutput()
{
}


bool TwoDOutput::useCoords() const
{
    return false;
}


bool TwoDOutput::wantsOutput( const TrcKey& tk ) const
{
    return seldata_->crlRange().includes( tk.trcNr(), true );
}


bool TwoDOutput::wantsOutput( const Coord& ) const
{
    pErrMsg( "Use wantsOutput with a TrcKey" );
    return false;
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
    mDynamicCastGet(Seis::RangeSelData*,rsd,seldata_);
    const float zstep = rsd ? rsd->cubeSampling().zsamp_.step_ : SI().zStep();
    tkzs.zsamp_ = StepInterval<float>( zrg.start_, zrg.stop_, zstep );
    return true;
}


bool TwoDOutput::doInit()
{
    const Interval<int> rg( seldata_->crlRange() );
    if ( rg.start_ <= 0 && Values::isUdf(rg.stop_) )
	seldata_->setIsAll( true );

    return true;
}


void TwoDOutput::collectData( const DataHolder& data, float refstep,
			      const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size() )
	return;

    if ( !output_ )
	return;

    output_->dataset_ += data.clone();

    SeisTrcInfo* trcinfo = new SeisTrcInfo(info);
    trcinfo->sampling_.step_ = refstep;
    //trcinfo->sampling.start = 0;
    output_->trcinfoset_ += trcinfo;
}


void TwoDOutput::setOutput( Data2DHolder& no )
{
    output_ = &no;
}


TypeSet< Interval<int> > TwoDOutput::getLocalZRanges( const TrcKey&,
						      float zstep,
						      TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<float> zrg( seldata_->zRange() );
        Interval<int> interval( mNINT32(zrg.start_/zstep),
				mNINT32(zrg.stop_/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }

    return sampleinterval_;
}


TypeSet< Interval<int> > TwoDOutput::getLocalZRanges( const Coord&,
						      float zstep,
						      TypeSet<float>& set) const
{
    return getLocalZRanges( TrcKey(), zstep, set );
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
    const BinID bid = SI().transform(info.coord_);
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


bool LocationOutput::useCoords() const
{
    return false;
}


bool LocationOutput::wantsOutput( const TrcKey& tk ) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( tk.binID() );
    return pos.isValid();
}


bool LocationOutput::wantsOutput( const Coord& ) const
{
    pErrMsg( "Use wantsOutput with TrcKey" );
    return false;
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const TrcKey& tk, float zstep,
						TypeSet<float>& exactz) const
{
    //TODO not 100% optimized, case of picksets for instance->find better algo
    TypeSet< Interval<int> > sampleinterval;

    BinIDValueSet::SPos pos = bidvalset_.find( tk.binID() );
    while ( pos.isValid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	int zidx;
	DataHolder::getExtraZAndSampIdxFromExactZ( vals[0], zstep, zidx );
	Interval<int> interval( zidx, zidx );
	if ( arebiddupl_ )
	{
	    interval.start_ = zidx - 1;
	    interval.stop_ =  zidx + 2;
	}

	const bool intvadded = sampleinterval.addIfNew( interval );
	if ( intvadded )
	    exactz += vals[0];

	bidvalset_.next( pos );
	if ( tk.binID() != bidvalset_.getBinID(pos) )
	    break;
    }

    return sampleinterval;
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const Coord&, float ,
						TypeSet<float>& ) const
{
    TypeSet< Interval<int> > sampleinterval;
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
    , outval_(outval)
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
        float val = bidvalset.valRange(idx).start_;
	if ( val < zmin ) zmin = val;
        val = bidvalset.valRange(idx+1).stop_;
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
	trc->info().sampling_.start_ = trcstarttime;
	trc->info().sampling_.step_ = refstep;
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


bool TrcSelectionOutput::useCoords() const
{
    return false;
}


bool TrcSelectionOutput::wantsOutput( const TrcKey& tk ) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( tk.binID() );
    return pos.isValid();
}


bool TrcSelectionOutput::wantsOutput( const Coord& crd ) const
{
    if ( is2d_ )
	return false;

    const BinID bid = SI().transform( crd );
    BinIDValueSet::SPos pos = bidvalset_.find( bid );
    return pos.isValid();
}


void TrcSelectionOutput::setOutput( SeisTrcBuf* outp_ )
{
    outpbuf_ = outp_;
    if ( outpbuf_ )
	outpbuf_->erase();
}


void TrcSelectionOutput::setTrcsBounds( const Interval<float>& intv )
{
    stdstarttime_ = intv.start_;
    stdtrcsz_ = intv.stop_ - intv.start_;
    seldata_->setZRange( intv );
}


void TrcSelectionOutput::setGeomID( const Pos::GeomID& geomid )
{
    seldata_->setGeomID( geomid );
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const TrcKey& tk, float zstep,
						TypeSet<float>&	) const
{
    BinIDValueSet::SPos pos = bidvalset_.find( tk.binID() );
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


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const Coord& crd, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;
    if ( is2d_ )
	return sampleinterval;

    const TrcKey tk( SI().transform(crd) );
    return getLocalZRanges( tk, zstep, exactz );
}


bool TrcSelectionOutput::getDesiredVolume( TrcKeyZSampling& cs ) const
{
    Interval<int> inlrg = bidvalset_.inlRange();
    Interval<int> crlrg = bidvalset_.crlRange();
    Interval<float> zrg =
	Interval<float>( stdstarttime_, stdstarttime_ + stdtrcsz_ );
    TrcKeyZSampling trcselsampling( false );
    trcselsampling.include ( BinID( inlrg.start_, crlrg.start_), zrg.start_);
    trcselsampling.include ( BinID( inlrg.stop_, crlrg.stop_), zrg.stop_);
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
Trc2DVarZStorOutput::Trc2DVarZStorOutput( const Pos::GeomID& geomid,
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
        float val = poszvalues_->bivSet().valRange( z1colidx ).start_;
	if ( val < zmin ) zmin = val;
        val = poszvalues_->bivSet().valRange( z2colidx ).stop_;
	if ( val > zmax ) zmax = val;
    }

    setGeometry( getCS() );
    seldata_->setZRange( Interval<float>(zmin,zmax) );
    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


Trc2DVarZStorOutput::~Trc2DVarZStorOutput()
{}


void Trc2DVarZStorOutput::setTrcsBounds( const Interval<float>& intv )
{
    stdstarttime_ = intv.start_;
    stdtrcsz_ = intv.stop_ - intv.start_;
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
	    deleteAndNullPtr( auxpars_ );
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
	trc_->info().sampling_.step_ = refstep;
	trc_->info().sampling_.start_ = trcstarttime;
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
						const TrcKey& tk, float zstep,
						TypeSet<float>& ) const
{
    TypeSet< Interval<int> > sampleinterval;
    DataPointSet::RowID rowid = poszvalues_->findFirst( tk );
    if ( rowid < 0 )
	return sampleinterval;

    Interval<int> interval( mNINT32(poszvalues_->z(rowid)/zstep),
			    mNINT32(poszvalues_->value(0,rowid)/zstep) );
    sampleinterval += interval;
    return sampleinterval;
}


TypeSet< Interval<int> > Trc2DVarZStorOutput::getLocalZRanges(
						const Coord&, float,
						TypeSet<float>& ) const
{
    pErrMsg( "Use getLocalZRanges with TrcKey");
    TypeSet< Interval<int> > sampleinterval;
    return sampleinterval;
}


bool Trc2DVarZStorOutput::useCoords() const
{
    return false;
}


bool Trc2DVarZStorOutput::wantsOutput( const TrcKey& tk ) const
{
    return poszvalues_->findFirst( tk ) > -1;
}


bool Trc2DVarZStorOutput::wantsOutput( const Coord& ) const
{
    pErrMsg( "Use wantsOutput with TrcKey" );
    return false;
}


bool Trc2DVarZStorOutput::finishWrite()
{
    return writer_->close();
}



// TableOutput
TableOutput::TableOutput( DataPointSet& datapointset, int firstcol )
    : dps_(&datapointset)
    , firstattrcol_(firstcol)
    , mediandisttrcs_(mUdf(float))
{
    ensureSelType( Seis::Table );
    seldata_->setIsAll( false );
    ((Seis::TableSelData*)seldata_)->binidValueSet().allowDuplicateBinIDs(true);
    ((Seis::TableSelData*)seldata_)->binidValueSet() = dps_->bivSet();

    arebiddupl_ = areBIDDuplicated();
}


TableOutput::~TableOutput()
{}


bool TableOutput::getDesiredVolume( TrcKeyZSampling& tkzs ) const
{
    return dps_->getRange( tkzs );
}


void TableOutput::setMedianDistBetwTrcs( float mediandist )
{
    mediandisttrcs_ = mediandist;
}


void TableOutput::collectData( const DataHolder& data, float refstep,
			       const SeisTrcInfo& info )
{
    const TrcKey tkey = info.trcKey();
    const BinID dpsbid = tkey.geomSystem() == dps_->geomSystem() ?
			tkey.position() : SI().transform( tkey.getCoord() );
    DataPointSet::RowID rid = dps_->findFirst( dpsbid );
    if ( rid < 0 )
	return;

    const int desnrvals = desoutputs_.size() + firstattrcol_;
    if ( dps_->nrCols() < desnrvals )
	dps_->bivSet().setNrVals(desnrvals+dps_->nrFixedCols());

    if ( !arebiddupl_ )
    {
	float* vals = dps_->getValues( rid );
	computeAndSetVals( data, refstep, dps_->z(rid), vals );
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    for ( int idx=rid; idx<dps_->size(); idx++ )
    {
	if ( dps_->binID(idx) != dpsbid )
	    break;

	const float zval = dps_->z(idx);
	float* vals = dps_->getValues( idx );
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


bool TableOutput::wantsOutput( const Coord& coord ) const
{
    return dps_->findFirst( coord ) > -1;
}


bool TableOutput::wantsOutput( const TrcKey& tkey ) const
{
    return tkey.geomSystem() == dps_->geomSystem() ?
			dps_->findFirst( tkey ) > -1 :
			dps_->findFirst( tkey.getCoord() ) > -1;
}


bool TableOutput::useCoords() const
{
    return false;
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const Coord& coord, float zstep,
						TypeSet<float>& exactz) const
{
    TrcKey tkey;
    tkey.setGeomSystem( dps_->geomSystem() ).setFrom( coord );
    return getLocalZRanges( tkey, zstep, exactz );
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const TrcKey& tkey, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;
    if ( tkey.isUdf() )
	return sampleinterval;

    const BinID dpsbid = tkey.geomSystem() == dps_->geomSystem() ?
			tkey.position() : SI().transform( tkey.getCoord() );
    DataPointSet::RowID rid = dps_->findFirst( dpsbid );
    if ( rid < 0 )
	return sampleinterval;

    addLocalInterval( sampleinterval, exactz, rid, zstep );
    for ( int idx=rid+1; idx<dps_->size(); idx++ )
    {
	if ( dps_->binID(idx) != dpsbid )
	    break;

	addLocalInterval( sampleinterval, exactz, idx, zstep );
    }

    return sampleinterval;
}


void TableOutput::addLocalInterval( TypeSet< Interval<int> >& sampintv,
				    TypeSet<float>& exactz,
				    int rid, float zstep ) const
{
    const float zval = dps_->z(rid);
    int zidx;
    DataHolder::getExtraZAndSampIdxFromExactZ( zval, zstep, zidx );
    Interval<int> interval( zidx, zidx );

    //Necessary if bid are duplicated and for a chain of attribs with stepout
    interval.start_ = zidx - 1;
    interval.stop_ =  zidx + 2;

    bool intvadded = sampintv.addIfNew( interval );
    if ( intvadded )
	exactz += zval;
}


bool TableOutput::areBIDDuplicated() const
{
    if ( dps_->isEmpty() ) return false;

    BinID prevbid( dps_->binID(0) );
    for ( int idx=1; idx<dps_->size(); idx++ )
    {
	const BinID bid( dps_->binID(idx) );
	if ( bid == prevbid ) return true;
	prevbid = bid;
    }

    return false;
}

} // namespace Attrib
