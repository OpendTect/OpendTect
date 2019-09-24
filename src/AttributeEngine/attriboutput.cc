/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/



#include "attriboutput.h"

#include "attribdataholder.h"
#include "convmemvalseries.h"
#include "cubesubsel.h"
#include "datapointset.h"
#include "linesubsel.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisrangeseldata.h"
#include "seistableseldata.h"
#include "seistrc.h"
#include "seistype.h"
#include "seisstorer.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom.h"
#include "uistrings.h"
#include "keystrs.h"

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
    : seldata_(new Seis::RangeSelData)
{
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
    if ( !seldata_ )
	seldata_ = Seis::SelData::get( st );
    else if ( seldata_->type() != st )
    {
	auto* newseldata = Seis::SelData::get( st );
	newseldata->copyFrom( *seldata_ );
	delete seldata_; seldata_ = newseldata;
    }
}


void Output::doSetGeometry( const FullSubSel& fss )
{
    if ( !fss.isAll() )
	{ delete seldata_; seldata_ = new Seis::RangeSelData( fss ); }
}


DataPackOutput::DataPackOutput( const FullSubSel& fss )
    : desiredsubsel_(fss)
    , dcfss_(fss)
    , output_(0)
    , udfval_(mUdf(float))
{
}


DataPackOutput::DataPackOutput( const TrcKeyZSampling& cs )
    : desiredsubsel_(cs)
    , dcfss_(cs)
    , output_(0)
    , udfval_(mUdf(float))
{
}


bool DataPackOutput::is2D() const
{
    return desiredsubsel_.is2D();
}


bool DataPackOutput::getDesiredSubSel( FullSubSel& fss ) const
{
    fss = desiredsubsel_;
    return true;
}


bool DataPackOutput::wantsOutput( const BinID& bid ) const
{
    if ( is2D() )
	return desiredsubsel_.lineSubSel(0).trcNrSubSel().includes(bid.crl());
    else
	return desiredsubsel_.cubeSubSel().includes( bid );
}


TypeSet<Interval<int> > DataPackOutput::getLocalZRanges( const BinID&,
							 float zstep,
							 TypeSet<float>& ) const
{
    if ( sampleinterval_.size() ==0 )
    {
	const auto zrg = desiredsubsel_.zRange();
	Interval<int> interval( mNINT32( zrg.start / zstep ),
				mNINT32( zrg.stop / zstep ) );
	const_cast<DataPackOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


void DataPackOutput::setPossibleSubSel( const FullSubSel& possss )
{
    desiredsubsel_.limitTo( possss );
    dcfss_ = desiredsubsel_;
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

    const auto& subsel = output_->subSel();
    if ( !subsel.includes(info.trcKey()) )
	return;

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
	if ( desout >= output_->nrComponents() )
	    output_->addComponent( OD::EmptyString(), true );

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

    const auto zrg = subsel.zRange();
    const auto& hss = subsel.horSubSel();

    const Interval<int> inputrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    const float z0 = zrg.start / zrg.step;
    const int outz0samp = mNINT32( z0 );
    const Interval<int> outrg( outz0samp, outz0samp+zrg.nrSteps() );
    if ( !inputrg.overlaps(outrg,false) )
	return;

    const Interval<int> transrg( mMAX(inputrg.start, outrg.start),
				 mMIN(inputrg.stop, outrg.stop ) );
    const auto lineidx = hss.idx4LineNr( info.lineNr());
    const auto trcidx = hss.idx4TrcNr( info.trcNr() );

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	const ValueSeries<float>* vals = data.series( desoutputs_[desout] );
	if ( !vals )
	    continue;

	Array3D<float>& outarr3d = output_->data( desout );
	const int zarrsz = outarr3d.getSize( 2 );

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
    output_ = new RegularSeisDataPack( OD::EmptyString(), bdd );
    output_->setSampling( TrcKeyZSampling(dcfss_) );
    DPM(DataPackMgr::SeisID()).add( output_ );
    auto& zss = output_->zSubSel();
    auto zrg = zss.zRange();
    zrg.step = refstep;
    zss.setOutputZRange( zrg );
}


SeisTrcStorOutput::SeisTrcStorOutput( const FullSubSel& fss )
    : desiredsubsel_(fss)
    , auxpars_(0)
    , storid_(*new DBKey)
    , storer_(0)
    , trc_(0)
    , prevpos_(-1,-1)
    , storinited_(false)
    , errmsg_(uiString::empty())
    , scaler_(0)
    , growtrctosi_(false)
    , writez0shift_(0.f)
{
    if ( geomID().isValid() && seldata_->isRange() )
	seldata_->asRange()->setGeomID( geomID() );
}


bool SeisTrcStorOutput::getDesiredSubSel( FullSubSel& fss ) const
{
    fss = desiredsubsel_;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredsubsel_.is2D()
	 ? desiredsubsel_.lineSubSel(0).trcNrSubSel().includes(bid.crl())
	 : desiredsubsel_.cubeSubSel().includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const DBKey& storid )
{
    if ( storid.isValid() )
    {
	PtrMan<IOObj> ioseisout = getIOObj( storid );
	if ( !ioseisout )
	{
	    errmsg_ = tr("Cannot find seismic data with ID: %1").arg( storid );
	    return false;
	}
    }

    storid_ = storid;
    return true;
}


SeisTrcStorOutput::~SeisTrcStorOutput()
{
    delete storer_;
    delete &storid_;
    delete auxpars_;
    delete scaler_;
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars, int outidx )
{
    errmsg_ = uiString::empty();
    PtrMan<IOPar> outppar =
		pars.subselect( IOPar::compKey(sKey::Output(),outidx) );
    if ( !outppar )
    {
	errmsg_ = uiStrings::phrCannotFind(
			tr("Output.%1 keyword in parameter file").arg(outidx) );
	return false;
    }

    const char* storid = outppar->find( seisidkey() );
    const DBKey dbky( storid );
    if ( !setStorageID(dbky) )
    {
	errmsg_ = uiStrings::phrCannotFind(tr("Output ID: %1").arg( storid ));
        return false;
    }

    if ( dbky.hasAuxKey() )
    {
	SeparString sepstr( dbky.auxKey(), '|' );
	attribname_ = sepstr[0];
    }

    const char* res = outppar->find( scalekey() );
    if ( res )
    {
	scaler_ = Scaler::get( res );
	if ( scaler_ && scaler_->isEmpty() )
	    { delete scaler_; scaler_ = 0; }
    }

    auxpars_ = pars.subselect("Aux");
    return doInit();

}


bool SeisTrcStorOutput::doInit()
{
    if ( storid_.isValid() )
    {
	PtrMan<IOObj> ioseisout = getIOObj( storid_ );
	if ( !ioseisout )
	    { errmsg_ = uiStrings::phrCannotFindDBEntry(storid_); return false;}

	storer_ = new Seis::Storer( *ioseisout );
	is2d_ = storer_->is2D();

	if ( auxpars_ )
	{
	    storer_->auxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

    if ( !is2d_ && seldata_->isRange() )
	desiredsubsel_.limitTo( seldata_->asRange()->fullSubSel() );

    return true;
}


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
	trc_->info().sampling_.step = refstep;
	trc_->info().sampling_.start = data.z0_*refstep + writez0shift_;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }
    else if ( trc_->info().binID() != info.binID() )
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

    auto reqzrg = desiredsubsel_.zRange();
    if ( !mIsEqual(reqzrg.step,trc_->info().sampling_.step,1e-6))
    {
	reqzrg.limitTo( trc_->zRange() );
	const int nrsamps = mCast( int, reqzrg.nrfSteps() + 1 );
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    SeisTrc temptrc( *trc_ );
	    trc_->info().sampling_.step = reqzrg.step;
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
    if ( !storer_ || !trc_ )
	return true;

    SeisTrc* usetrc = trc_;
    PtrMan<SeisTrc> tmptrc = 0;
    if ( growtrctosi_ )
    {
	tmptrc = trc_->getExtendedTo( SI().zRange(), true );
	usetrc = tmptrc;
    }

    if ( !storinited_ )
    {
	SeisTrcTranslator* transl = 0;
	if ( !storer_->is2D() )
	{
	    transl = storer_->translator();
	    if ( transl )
		transl->setComponentNames( outpnames_ );
	}

	auto uirv = storer_->prepareWork( *usetrc );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; return false; }

	if ( storer_->is2D() )
	{
	    if ( storer_->linePutter() )
	    {
		mDynamicCastGet( SeisCBVS2DLinePutter*, lp,
				 storer_->linePutter() )
		if ( lp && lp->tr_ )
		    lp->tr_->setComponentNames( outpnames_ );
	    }
	}

	if ( transl && !outptypes_.isEmpty() )
	    transl->setDataType( outptypes_[0] );

	storinited_ = true;
    }

    errmsg_ = storer_->put( *usetrc );
    delete trc_; trc_ = 0;
    return errmsg_.isEmpty();
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRanges(
						    const BinID& bid,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	const auto zrg = desiredsubsel_.zRange();
	Interval<int> interval( mNINT32(zrg.start/zstep),
				mNINT32(zrg.stop/zstep) );
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
    errmsg_ = storer_->close();
    return errmsg_.isEmpty();
}


TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			Pos::GeomID geomid )
    : errmsg_(uiString::empty())
    , output_( 0 )
{
    delete seldata_;
    seldata_ = new Seis::RangeSelData( geomid );
    if ( trg.start>0 && !mIsUdf(trg.start) && !mIsUdf(trg.stop) )
	setGeometry( trg, zrg );
}


TwoDOutput::~TwoDOutput()
{
    if ( output_ )
	output_->unRef();
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_->trcNrRange(0).includes(bid.crl(),true);
}


void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    seldata_->asRange()->setZRange( zrg );
    seldata_->asRange()->setTrcNrRange( trg );
}


bool TwoDOutput::getDesiredSubSel( FullSubSel& fss ) const
{
    fss = FullSubSel( seldata_->geomID(0) );
    fss.setTrcNrRange( seldata_->trcNrRange(0) );
    fss.setZRange( seldata_->zRange(0) );
    return true;
}


bool TwoDOutput::doInit()
{
    const Interval<int> rg( seldata_->trcNrRange(0) );
    if ( rg.start <= 0 || Values::isUdf(rg.stop) )
    {
	const auto geomid = seldata_->geomID(0);
	delete seldata_;
	seldata_ = new Seis::RangeSelData( geomid );
    }

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

    SeisTrcInfo* trcinfo = new SeisTrcInfo( info );
    trcinfo->sampling_.step = refstep;
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
	Interval<float> zrg( seldata_->zRange(0) );
	Interval<int> interval( mNINT32(zrg.start/zstep),
				mNINT32(zrg.stop/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


LocationOutput::LocationOutput( BinnedValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    deleteAndZeroPtr( seldata_ );
    ensureSelType( Seis::Table );
    auto& tsd = *seldata_->asTable();
    tsd.binidValueSet() = bidvalset;
    tsd.binidValueSet().allowDuplicatePositions( true );

    arebiddupl_ = areBIDDuplicated();
}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    BinnedValueSet::SPos pos = bidvalset_.find( info.binID() );
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
	if ( info.binID() != bidvalset_.getBinID(pos) )
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
    BinnedValueSet::SPos pos = bidvalset_.find( bid );
    return pos.isValid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz) const
{
    //TODO not 100% optimized, case of picksets for instance->find better algo
    TypeSet< Interval<int> > sampleinterval;

    BinnedValueSet::SPos pos = bidvalset_.find( bid );
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
    BinnedValueSet tmpset(bidvalset_);
    tmpset.allowDuplicatePositions( false );

    return tmpset.totalSize()<bidvalset_.totalSize();
}

TrcSelectionOutput::TrcSelectionOutput( const BinnedValueSet& bidvalset,
					float outval )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
    , outval_(outval)
    , stdstarttime_(mUdf(float))
    , stdtrcsz_(mUdf(float))
{
    delete seldata_;
    auto& tsd = *new Seis::TableSelData( bidvalset );
    seldata_ = &tsd;
    tsd.binidValueSet().allowDuplicatePositions( true );

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
	BinnedValueSet::SPos pos; bidvalset.next( pos );
	const BinID bid0( bidvalset.getBinID(pos) );
	tsd.binidValueSet().add( bid0, zmin );
	tsd.binidValueSet().add( bid0, zmax );

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
	trc->info().sampling_.start = trcstarttime;
	trc->info().sampling_.step = refstep;
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
    BinnedValueSet::SPos pos = bidvalset_.find( bid );
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
    if ( seldata_ && geomid.isValid() && seldata_->isRange() )
	seldata_->asRange()->setGeomID( geomid );
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>&	) const
{
    BinnedValueSet::SPos pos = bidvalset_.find( bid );
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


bool TrcSelectionOutput::getDesiredSubSel( FullSubSel& fss ) const
{
    fss.setToAll( false );

    auto css = fss.cubeSubSel();
    StepInterval<int> inlrg( bidvalset_.inlRange(), css.inlSubSel().posStep() );
    StepInterval<int> crlrg( bidvalset_.crlRange(), css.crlSubSel().posStep() );
    StepInterval<float> zrg( stdstarttime_, stdstarttime_ + stdtrcsz_,
			     css.zSubSel().zStep() );
    css.setInlRange( inlrg );
    css.setCrlRange( crlrg );
    css.setZRange( zrg );
    fss.set( css );
    return true;
}


Trc2DVarZStorOutput::Trc2DVarZStorOutput( Pos::GeomID geomid,
					  DataPointSet* poszvalues,
					  float outval )
    : SeisTrcStorOutput( FullSubSel(geomid) )
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

    setGeometry( desiredsubsel_ );
    seldata_->setZRange( Interval<float>(zmin,zmax) );
    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


void Trc2DVarZStorOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
}


bool Trc2DVarZStorOutput::doInit()
{
    if ( storid_.isValid() )
    {
	PtrMan<IOObj> ioseisout = getIOObj( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = tr("Cannot find seismic data with ID: %1").arg( storid_ );
	    return false;
	}

	storer_ = new Seis::Storer( *ioseisout );
	if ( !storer_->is2D() )
	{
	    errmsg_ = tr("Seismic data with ID: %1 is not 2D\n"
			 "Cannot create 2D output.").arg( storid_ );
	    return false;
	}

	if ( auxpars_ )
	{
	    storer_->auxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

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
	trc_->info().sampling_.step = refstep;
	trc_->info().sampling_.start = trcstarttime;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( trcsz, dc, false );
    }
    else if ( trc_->info().binID() != info.binID() )
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
		const float distn =
			coord.distTo<float>( poszvalues_->coord(idx) );
		const float distnp1 =
			coord.distTo<float>( poszvalues_->coord(idx+1));
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
	if ( mIsEqual( poszvalues_->coord(idx).x_, coord.x_, 1e-3 )
	   &&mIsEqual( poszvalues_->coord(idx).y_, coord.y_, 1e-3 ) )
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
    Coord roundedcoord( (int)coord.x_, (int)coord.y_ );
    return poszvalues_->findFirst( roundedcoord ) > -1;
}


bool Trc2DVarZStorOutput::finishWrite()
{
    errmsg_ = storer_->close();
    return errmsg_.isEmpty();
}


TableOutput::TableOutput( DataPointSet& datapointset, int firstcol )
    : datapointset_(datapointset)
    , firstattrcol_(firstcol)
{
    deleteAndZeroPtr( seldata_ );
    ensureSelType( Seis::Table );
    auto& tsd = *seldata_->asTable();
    tsd.binidValueSet().allowDuplicatePositions( true );
    tsd.binidValueSet() = datapointset_.bivSet();

    arebiddupl_ = areBIDDuplicated();
    distpicktrc_ = TypeSet<float>( datapointset.size(), mUdf(float) );
}


bool TableOutput::useCoords( OD::GeomSystem gs ) const
{
    return datapointset_.bivSet().geomSystem() != gs;
}

void TableOutput::collectData( const DataHolder& data, float refstep,
			       const SeisTrcInfo& info )
{
    const bool usecoords = useCoords( info.geomSystem() );
    const Coord coord = info.coord_;
    DataPointSet::RowID rid = usecoords ? datapointset_.findFirst(coord)
				      : datapointset_.findFirst(info.binID());
    if ( rid< 0 && datapointset_.is2D() )
    {
	//TODO remove when datapointset is snapped
	for ( int idx=0; idx<datapointset_.size()-1; idx++ )
	{
	    if ( coord > datapointset_.coord(idx) &&
		 coord < datapointset_.coord(idx+1) )
	    {
		const float distn =
			coord.distTo<float>( datapointset_.coord(idx) );
		const float distnp1 =
			coord.distTo<float>(datapointset_.coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 )
		    { rid = idx; break; }
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 )
		    { rid = idx+1; break; }
	    }
	}
    }

    if ( rid<0 ) return;

    if ( datapointset_.is2D() )
    {
	BinnedValueSet::SPos spos = datapointset_.bvsPos( rid );
	float* vals = datapointset_.bivSet().getVals( spos );
	vals[datapointset_.nrFixedCols()-1] = (float)info.trcKey().trcNr();
    }

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
	const BinID trcbid = usecoords ? SI().transform(info.coord_)
					: info.binID();
	if ( trcbid != datapointset_.binID(idx) ) break;

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
    BinnedValueSet::SPos pos = datapointset_.bivSet().find( bid );
    return pos.isValid();
}


bool TableOutput::wantsOutput( const Coord& coord ) const
{
    return datapointset_.findFirst( coord ) > -1;
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;

    const BinnedValueSet& bvs = datapointset_.bivSet();
    BinnedValueSet::SPos pos = bvs.find( bid );

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
    TableOutput* myself = const_cast<TableOutput*>(this);
    DataPointSet::RowID rid = datapointset_.findFirst( coord );
    if ( rid< 0 )
    {
	for ( int idx=0; idx<datapointset_.size()-1; idx++ )
	{
	    if ( coord > datapointset_.coord(idx) &&
		 coord < datapointset_.coord(idx+1) )
	    {
		const float distn =
			coord.distTo<float>( datapointset_.coord(idx) );
		const float distnp1 =
			coord.distTo<float>(datapointset_.coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 &&
		    ( mIsUdf(distpicktrc_[idx]) || distn<distpicktrc_[idx]) )
		{
		    rid = idx;
		    myself->distpicktrc_[idx] = mCast(float,distn);
		    break;
		}
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 &&
		  (mIsUdf(distpicktrc_[idx+1]) || distnp1<distpicktrc_[idx+1]) )
		{
		    rid = idx+1;
		    myself->distpicktrc_[idx+1] = mCast(float,distnp1);
		    break;
		}
	    }
	}
    }
    else
    {
	const float dist = coord.distTo<float>(datapointset_.coord(rid));
	myself->distpicktrc_[rid] = dist;
    }

    TypeSet< Interval<int> > sampleinterval;
    if ( rid< 0 ) return sampleinterval;

    Coord truecoord = datapointset_.coord( rid );
    for ( int idx=rid; idx<datapointset_.size(); idx++ )
    {
	if ( truecoord != datapointset_.coord( idx ) ) break;
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
