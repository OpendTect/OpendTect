/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.60 2007-09-14 14:11:49 cvshelene Exp $";

#include "attriboutput.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "convmemvalseries.h"
#include "interpol1d.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "simpnumer.h"
#include "survinfo.h"


namespace Attrib
{

const char* Output::outputstr = "Output";
const char* Output::cubekey = "Cube";
const char* Output::surfkey = "Surface";
const char* Output::tskey = "Trace Selection";
const char* Output::scalekey = "Scale";

const char* SeisTrcStorOutput::seisidkey = "Seismic ID";
const char* SeisTrcStorOutput::attribkey = "Attributes";
const char* SeisTrcStorOutput::inlrangekey = "In-line range";
const char* SeisTrcStorOutput::crlrangekey = "Cross-line range";
const char* SeisTrcStorOutput::depthrangekey = "Depth range";

const char* LocationOutput::filenamekey = "Output Filename";
const char* LocationOutput::locationkey = "Locations";
const char* LocationOutput::attribkey = "Attribute";
const char* LocationOutput::surfidkey = "Surface ID";


Output::Output()
    : seldata_(*new SeisSelData)
{}


Output::~Output()
{
    delete &seldata_;
}


const LineKey& Output::curLineKey() const
{
    return seldata_.linekey_;
}


DataCubesOutput::DataCubesOutput( const CubeSampling& cs )
    : desiredvolume_(cs)
    , datacubes_(0)
    , udfval_(mUdf(float))
{
}


DataCubesOutput::~DataCubesOutput()
{ if ( datacubes_ ) datacubes_->unRef(); }


bool DataCubesOutput::getDesiredVolume( CubeSampling& cs ) const
{ cs=desiredvolume_; return true; }


bool DataCubesOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume_.hrg.includes(bid); }


TypeSet< Interval<int> > DataCubesOutput::getLocalZRanges( const BinID&,
							   float zstep,
       							   TypeSet<float>&)const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT( desiredvolume_.zrg.start / zstep ),
				mNINT( desiredvolume_.zrg.stop / zstep ) );
	const_cast<DataCubesOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_; 
}


void DataCubesOutput::adjustInlCrlStep( const CubeSampling& cs )
{
    if ( cs.hrg.step.inl > desiredvolume_.hrg.step.inl )
	desiredvolume_.hrg.step.inl = cs.hrg.step.inl;
    if ( cs.hrg.step.crl > desiredvolume_.hrg.step.crl )
	desiredvolume_.hrg.step.crl = cs.hrg.step.crl;
}


#define mGetSz(dir)\
	dir##sz = (desiredvolume_.hrg.stop.dir - desiredvolume_.hrg.start.dir)\
		  /desiredvolume_.hrg.step.dir + 1;\

#define mGetZSz()\
	zsz = mNINT( ( desiredvolume_.zrg.stop - desiredvolume_.zrg.start )\
	      /refstep + 1 );

void DataCubesOutput::collectData( const DataHolder& data, float refstep, 
				  const SeisTrcInfo& info )
{
    if ( !datacubes_ )
	init( refstep );

    if ( !datacubes_->includes(info.binid) )
	return;
		
    const int totalnrcubes = desoutputs_.size();
    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	if ( desout<datacubes_->nrCubes() )
	    continue;

	mDynamicCastGet( ConvMemValueSeries<float>*, cmvs,
			 data.series(desoutputs_[desout]) );

	if ( cmvs && cmvs->handlesUndef() )
	{
	    const BinDataDesc desc = cmvs->dataDesc();
	    datacubes_->addCube(mUdf(float), false, &desc );
	}
	else
	    datacubes_->addCube(mUdf(float));
    }

    int zsz; 
    mGetZSz();

    const Interval<int> inputrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    const Interval<int> outrg( datacubes_->z0, datacubes_->z0+zsz-1 );

    if ( !inputrg.overlaps(outrg,false) )
	return;

    const Interval<int> transrg( mMAX(inputrg.start, outrg.start),
	    			 mMIN(inputrg.stop, outrg.stop ) );

    const int inlidx =
	datacubes_->inlsampling.nearestIndex(info.binid.inl);
    const int crlidx =
	datacubes_->crlsampling.nearestIndex(info.binid.crl);

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	if ( !data.series(desoutputs_[desout]) )
	    continue;

	mDynamicCastGet( ConvMemValueSeries<float>*, cmvs,
			 data.series(desoutputs_[desout]) );

	if ( !cmvs )
	{
	    for ( int idx=transrg.start; idx<=transrg.stop; idx++)
	    {
		const float val =
		    data.series(desoutputs_[desout])->value(idx-data.z0_);

		datacubes_->setValue( desout, inlidx, crlidx,
				      idx-datacubes_->z0, val);
	    }
	}
	else
	{
	    mDynamicCastGet( ConvMemValueSeries<float>*, deststor,
		    	     datacubes_->getCube(desout).getStorage() );
	    const char elemsz = cmvs->dataDesc().nrBytes();

	    const od_int64 destoffset = transrg.start-datacubes_->z0 +
		datacubes_->getCube(desout).info().getOffset(inlidx,crlidx,0);

	    char* dest = deststor->storArr() + destoffset * elemsz;
	    char* src = cmvs->storArr() +
			(transrg.start-data.z0_) * elemsz;
	    memcpy( dest, src, elemsz*(transrg.width()+1) );
	}
    }
}


const DataCubes* DataCubesOutput::getDataCubes() const
{
    return datacubes_;
}


DataCubes* DataCubesOutput::getDataCubes( float refstep )
{
    if ( !datacubes_ )
	init( refstep );

    return datacubes_;
}


void DataCubesOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


void DataCubesOutput::init( float refstep )
{
    datacubes_ = new Attrib::DataCubes;
    datacubes_->ref();
    datacubes_->inlsampling= StepInterval<int>(desiredvolume_.hrg.start.inl,
					       desiredvolume_.hrg.stop.inl,
					       desiredvolume_.hrg.step.inl);
    datacubes_->crlsampling= StepInterval<int>(desiredvolume_.hrg.start.crl,
					       desiredvolume_.hrg.stop.crl,
					       desiredvolume_.hrg.step.crl);
    datacubes_->z0 = mNINT(desiredvolume_.zrg.start/refstep);
    datacubes_->zstep = refstep;
    int inlsz, crlsz, zsz;
    mGetSz(inl); mGetSz(crl); mGetZSz();
    datacubes_->setSize( inlsz, crlsz, zsz );
}


SeisTrcStorOutput::SeisTrcStorOutput( const CubeSampling& cs,
				      const LineKey& lk )
    : desiredvolume_(cs)
    , auxpars_(0)
    , storid_(*new MultiID)
    , writer_(0)
    , trc_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , errmsg_(0)
    , scaler_(0)
{
    seldata_.linekey_ = lk;
    attribname_ = lk.attrName();
}


bool SeisTrcStorOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs = desiredvolume_;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredvolume_.hrg.includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const MultiID& storid )
{
    if ( *((const char*)storid) )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid;
	    return false;
	}
    }

    storid_ = storid;
    return true;
}


void SeisTrcStorOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


SeisTrcStorOutput::~SeisTrcStorOutput()
{
    delete writer_;
    delete &storid_;
    delete auxpars_;
    delete scaler_;
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars )
{
    errmsg_ = "";
    PtrMan<IOPar> outppar = pars.subselect("Output.1");
    const char* storid = outppar->find("Seismic ID");
    if ( !setStorageID( storid ) )
    {
        errmsg_ = "Could not find output ID: "; errmsg_ += storid;
        return false;
    }

    const char* res = outppar->find( scalekey );
    if ( res )
    {
	scaler_ = new LinScaler;
	scaler_->fromString( res );
	if ( scaler_->isEmpty() )
	    { delete scaler_; scaler_ = 0; }
    }
    
    auxpars_ = pars.subselect("Aux");
    doInit();
    return true;
}//warning, only a small part of the old taken, see if some more is required


bool SeisTrcStorOutput::doInit()
{
    if ( *storid_.buf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid_;
	    return false;
	}

	writer_ = new SeisTrcWriter( ioseisout );
	is2d_ = writer_->is2D();
	if ( auxpars_ )
	{
	    writer_->lineAuxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

    desiredvolume_.normalise();
    seldata_.linekey_.setAttrName( "" );
    if ( seldata_.type_ != Seis::Range )
	seldata_.type_ = Seis::Range;

    if ( !is2d_ )
    {
	if ( seldata_.inlrg_.start > desiredvolume_.hrg.start.inl )
	    desiredvolume_.hrg.start.inl = seldata_.inlrg_.start;
	if ( seldata_.inlrg_.stop < desiredvolume_.hrg.stop.inl )
	    desiredvolume_.hrg.stop.inl = seldata_.inlrg_.stop;
	if ( seldata_.crlrg_.start > desiredvolume_.hrg.start.crl )
	    desiredvolume_.hrg.start.crl = seldata_.crlrg_.start;
	if ( seldata_.crlrg_.stop < desiredvolume_.hrg.stop.crl )
	    desiredvolume_.hrg.stop.crl = seldata_.crlrg_.stop;
	if ( seldata_.zrg_.start > desiredvolume_.zrg.start )
	    desiredvolume_.zrg.start = seldata_.zrg_.start;
	if ( seldata_.zrg_.stop < desiredvolume_.zrg.stop )
	    desiredvolume_.zrg.stop = seldata_.zrg_.stop;
    }

    return true;
}


class COLineKeyProvider : public LineKeyProvider
{
public:

COLineKeyProvider( SeisTrcStorOutput& c, const char* a, const char* lk )
	: co_(c) , attrnm_(a) , linename_(lk) {}

LineKey lineKey() const
{
    LineKey lk(linename_,attrnm_);
    return lk;
}
    SeisTrcStorOutput&   co_;
    BufferString        attrnm_;
    BufferString 	linename_;

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
    else if ( trc_->info().binid != info.binid )
    {
	errmsg_ = "merge components of two different traces!";
	return;	    
    }
    else
    {
	for ( int idx=0; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.series(desoutputs_[comp])->value(idx);
	    trc_->set(idx, val, comp);
	}
    }
    
    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }
}


void SeisTrcStorOutput::writeTrc()
{
    if ( !trc_ ) return;
    
    if ( !storinited_ )
    {
	if ( writer_->is2D() )
	{
	    if ( attribname_ == "inl_dip" || attribname_ == "crl_dip" )
		attribname_ = sKey::Steering;
	    else if ( IOObj::isKey(attribname_) )
		attribname_ = IOM().nameOf(attribname_);

	    writer_->setLineKeyProvider( 
		new COLineKeyProvider( *this, attribname_, 
		    		       curLineKey().lineName()) );
	}

	if ( !writer_->prepareWork(*trc_) )
	    { errmsg_ = writer_->errMsg(); return; }

	SeisTrcTranslator* transl = writer_->seisTranslator();
	if ( transl && !writer_->is2D() )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& cis
		             = transl->componentInfo();
	    for ( int idx=0; idx<cis.size(); idx++ )
		cis[idx]->datatype = outptypes_.size() ? outptypes_[idx] : 
		    					Seis::UnknowData;
	}

	storinited_ = true;
    }
    
    if ( !writer_->put(*trc_) )
	{ errmsg_ = writer_->errMsg(); }

    delete trc_;
    trc_ = 0;
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRanges( 
						    const BinID& bid,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT(desiredvolume_.zrg.start/zstep), 
				mNINT(desiredvolume_.zrg.stop/zstep) );
	const_cast<SeisTrcStorOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			const LineKey& lk )
    : errmsg_(0)
    , output_( 0 )
{
    seldata_.linekey_ = lk;
    setGeometry( trg, zrg );
}


TwoDOutput::~TwoDOutput()
{
    if ( output_ ) output_->unRef();
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_.crlrg_.includes(bid.crl);
} 
 

void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    seldata_.zrg_ = zrg;
    seldata_.crlrg_.setFrom(trg);
    seldata_.type_ = Seis::Range;
}


bool TwoDOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs.hrg.start.crl = seldata_.crlrg_.start;
    cs.hrg.stop.crl = seldata_.crlrg_.stop;
    cs.zrg = StepInterval<float>( seldata_.zrg_.start, seldata_.zrg_.stop,
	    			  SI().zStep() );
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
    return true;
}


bool TwoDOutput::doInit()
{
    seldata_.linekey_.setAttrName( "" );
    if ( seldata_.crlrg_.start <= 0 && Values::isUdf(seldata_.crlrg_.stop) )
	seldata_.type_ = Seis::All;

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
    trcinfo->sampling.start = 0;
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
	Interval<int> interval( mNINT(seldata_.zrg_.start/zstep), 
				mNINT(seldata_.zrg_.stop/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
    , classstatus_(-1)
{
    seldata_.all_ = false;
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( true );
    seldata_.table_ = bidvalset;

    arebiddupl_ = areBIDDuplicated();
}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( info.binid );
    if ( !pos.valid() ) return;

    const int desnrvals = desoutputs_.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    if ( !arebiddupl_ )
    {
	float* vals = bidvalset_.getVals( pos );
	for ( int comp=0; comp<desoutputs_.size(); comp++ )
	    vals[comp+1] = data.series(desoutputs_[comp])->value(0);
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    bool isfirstz = true;
    float firstz;
    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	const int lowz = mNINT( (vals[0]/refstep)-0.5 );
	const int highz = mNINT( (vals[0]/refstep)+0.5 );
	bool isfulldataok = datarg.includes(lowz-1) && datarg.includes(highz+1);
	bool canusepartdata = data.nrsamples_<4 && datarg.includes(lowz) 
			      && datarg.includes(highz);
	if ( isfulldataok || canusepartdata )
	    computeAndSetVals( data, refstep, vals, firstz, isfirstz );

	bidvalset_.next( pos );
	if ( info.binid != bidvalset_.getBinID(pos) )
	    break;
    }
}


void LocationOutput::computeAndSetVals( const DataHolder& data, float refstep,
					float* vals, float& firstz,
					bool& isfirstz )
{
    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    const int lowz = mNINT( (vals[0]/refstep)-0.5 );
    const int highz = mNINT( (vals[0]/refstep)+0.5 );
    if ( isfirstz )
	firstz = vals[0];
    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	int serieidx = desoutputs_[comp];
	float p0 = lowz-1 < data.z0_ ? mUdf(float)
		      : data.series(serieidx)->value(lowz-1-data.z0_);
	float p1 = data.series(serieidx)->value(lowz-data.z0_);
	float p2 = data.series(serieidx)->value(highz-data.z0_);
	float p3 = !datarg.includes(highz+1) ? mUdf(float)
		      : data.series(serieidx)->value(highz+1-data.z0_);
	if ( classstatus_ ==-1 || classstatus_ == 1 )
	{
	    TypeSet<float> tset;
	    tset += p0; tset += p1; tset += p2; tset += p3;
	    classstatus_ = holdsClassValues( tset.arr(), 4 ) ? 1 : 0;
	}

	float val;
	if ( classstatus_ == 0 )
	{
	    float disttop1 = isfirstz ? 0 : vals[0]/refstep - firstz/refstep;
	    val = Interpolate::polyReg1DWithUdf( p0, p1, p2, p3, disttop1 );
	}
	else 
	    val = mNINT( (vals[0]/refstep) )==lowz ? p1 : p2;
	vals[comp+1] = val;
    }
    if ( isfirstz ) isfirstz = false;
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz) const
{
    //TODO not 100% optimized, case of picksets for instance->find better algo
    TypeSet< Interval<int> > sampleinterval;
    
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    while ( pos.valid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	int zidx = mNINT( (vals[0]/zstep) -0.5 ); 
	Interval<int> interval( zidx, zidx );
	if ( arebiddupl_ )
	{
	    interval.start = mNINT( (vals[0]/zstep) -0.5 )-1;
	    interval.stop =  mNINT( (vals[0]/zstep) +0.5 )+1;
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
    tmpset.allowDuplicateBids(false);
    
    return tmpset.totalSize()<bidvalset_.totalSize();
}

TrcSelectionOutput::TrcSelectionOutput( const BinIDValueSet& bidvalset,
					float outval )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
    , outval_(outval)
{
    seldata_.all_ = false;
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( bidvalset.totalSize()<2 );
    seldata_.table_.setNrVals( 1 );

    const int nrinterv = bidvalset.nrVals()/2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	float val = bidvalset.valRange(idx).start;
	if ( val < zmin ) zmin = val;
	val = bidvalset.valRange(idx+1).stop;
	if ( val > zmax ) zmax = val;
    }

    BinIDValueSet::Pos pos;
    bidvalset.next( pos );
    seldata_.table_.add( bidvalset.getBinID(pos), zmin );
    while ( bidvalset.next(pos) )
	seldata_.table_.add( bidvalset.getBinID(pos), zmax );

    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


TrcSelectionOutput::~TrcSelectionOutput()
{}


void TrcSelectionOutput::collectData( const DataHolder& data, float refstep,
				      const SeisTrcInfo& info )
{
    const int nrcomp = data.nrSeries();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs_.size() )
	return;

    const int trcsz = mNINT(stdtrcsz_/refstep) + 1;
    const float globalsttime = stdstarttime_;
    const float trcstarttime = ( (int)(globalsttime/refstep) +1 ) * refstep;
    const int startidx = data.z0_ - mNINT(trcstarttime/refstep);
    const int index = outpbuf_->find( info.binid );

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
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
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
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const BinID& bid, float zstep,
       						TypeSet<float>&	) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    BinID binid;
    TypeSet<float> values;
    bidvalset_.get( pos, binid, values );
    TypeSet< Interval<int> > sampleinterval;
    for ( int idx=0; idx<values.size()/2; idx+=2 )
    {
	Interval<int> interval( mNINT(values[idx]/zstep), 
				mNINT(values[idx+1]/zstep) );
	sampleinterval += interval;
    }
 
    return sampleinterval;
}

} // namespace Attrib
