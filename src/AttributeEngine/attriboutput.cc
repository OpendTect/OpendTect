/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.27 2005-09-15 09:06:17 cvshelene Exp $";

#include "attriboutput.h"
#include "attribdataholder.h"
#include "attribslice.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "seisbuf.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "linekey.h"


namespace Attrib
{

const char* Output::outputstr = "Output";
const char* Output::typekey = "Type";
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
{
    mRefCountConstructor;
}


Output::~Output()
{
    delete &seldata_;
}


SliceSetOutput::SliceSetOutput( const CubeSampling& cs )
    : desiredvolume(cs)
    , sliceset(0)
    , udfval(mUdf(float))
{
    const float dz = SI().zStep();
    Interval<int> interval( mNINT(cs.zrg.start/dz), mNINT(cs.zrg.stop/dz) );
    sampleinterval += interval;
}


SliceSetOutput::~SliceSetOutput()
{ if ( sliceset ) sliceset->unRef(); }


bool SliceSetOutput::getDesiredVolume( CubeSampling& cs ) const
{ cs=desiredvolume; return true; }


bool SliceSetOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


TypeSet< Interval<int> > SliceSetOutput::getLocalZRange( const BinID& ) const
{ return sampleinterval; }


#define mGetDim(nr) \
    const int dim##nr = sliceset->sampling.size( direction(sliceset->direction,nr) )

void SliceSetOutput::collectData( const DataHolder& data, float refstep, 
				  const SeisTrcInfo& info, int attridx )
{
    if ( !sliceset )
    {
	sliceset = new Attrib::SliceSet;
	sliceset->ref();
	sliceset->sampling = desiredvolume;
	sliceset->sampling.zrg.step = refstep; 
	sliceset->direction = desiredvolume.defaultDir();
    }
		
    mGetDim(0); mGetDim(1); mGetDim(2);
    const int totalnrslices = (desoutputs.size()+attridx) * dim0;
    while ( sliceset->size() < totalnrslices )
	*sliceset += new Attrib::Slice( dim1, dim2, udfval );

    if ( !sliceset->sampling.hrg.includes(info.binid) )
	return;

    int i0, i1, i2;
    int firstslicesample = mNINT(sliceset->sampling.zrg.start/refstep);
    const int nrz = sliceset->sampling.nrZ();
    Interval<int> dataidxrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    for ( int desout=0; desout<desoutputs.size(); desout++ )
    {
	for ( int idx=0; idx<nrz; idx++)
	{
	    const int dataidx = firstslicesample + idx;
	    float val = udfval;
	    if ( dataidxrg.includes(dataidx) )
		val = data.series(desoutputs[desout])->value(dataidx-data.z0_);

	    sliceset->getIdxs( info.binid.inl, info.binid.crl, dataidx*refstep, 
		    	       i0, i1, i2 );
	    const int slsetidx = (attridx+desout)*dim0 + i0;
	    ((*sliceset)[slsetidx])->set( i1, i2, val );
	}
    }
}


SliceSet* SliceSetOutput::getSliceSet() const
{
    return sliceset;
}


void SliceSetOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


SeisTrcStorOutput::SeisTrcStorOutput( const CubeSampling& cs , LineKey lkey )
    : desiredvolume(cs)
    , auxpars(0)
    , storid_(*new MultiID)
    , writer_(0)
    , calcurpos_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , errmsg(0)
{
    seldata_.linekey_ = lkey;

    const float dz = SI().zStep();
    Interval<int> interval( mNINT(cs.zrg.start/dz), mNINT(cs.zrg.stop/dz) );
    sampleinterval += interval;
}


bool SeisTrcStorOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs = desiredvolume;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredvolume.hrg.includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const MultiID& storid )
{
    if ( *((const char*)storid) )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid );
	if ( !ioseisout )
	{
	    errmsg = "Cannot find seismic data with ID: "; errmsg += storid;
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
    delete auxpars;
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars )
{
    errmsg = "";
    PtrMan<IOPar> outppar = pars.subselect("Output.1");
    const char* storid = outppar->find("Seismic ID");
    if ( !setStorageID( storid ) )
    {
        errmsg = "Could not find output ID: "; errmsg += storid;
        return false;
    }

    auxpars = pars.subselect("Aux");
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
	    errmsg = "Cannot find seismic data with ID: "; errmsg += storid_;
	    return false;
	}

	writer_ = new SeisTrcWriter( ioseisout );
	is2d_ = writer_->is2D();
	if ( auxpars )
	{
	    writer_->lineAuxPars().merge( *auxpars );
	    delete auxpars; auxpars = 0;
	}
    }

    desiredvolume.normalise();
    seldata_.linekey_.setAttrName( "" );
    if ( seldata_.type_ != Seis::Range )
	seldata_.type_ = Seis::Range;

    if ( !is2d_ )
    {
	if ( seldata_.inlrg_.start > desiredvolume.hrg.start.inl )
	    desiredvolume.hrg.start.inl = seldata_.inlrg_.start;
	if ( seldata_.inlrg_.stop < desiredvolume.hrg.stop.inl )
	    desiredvolume.hrg.stop.inl = seldata_.inlrg_.stop;
	if ( seldata_.crlrg_.start > desiredvolume.hrg.start.crl )
	    desiredvolume.hrg.start.crl = seldata_.crlrg_.start;
	if ( seldata_.crlrg_.stop < desiredvolume.hrg.stop.crl )
	    desiredvolume.hrg.stop.crl = seldata_.crlrg_.stop;
	if ( seldata_.zrg_.start > desiredvolume.zrg.start )
	    desiredvolume.zrg.start = seldata_.zrg_.start;
	if ( seldata_.zrg_.stop < desiredvolume.zrg.stop )
	    desiredvolume.zrg.stop = seldata_.zrg_.stop;
    }

    return true;
}


bool SeisTrcStorOutput::setReqs( const BinID& pos )
{
    calcurpos_ = true;
    if ( !is2d_ )
    {
	if ( pos == prevpos_ )
	{
	    calcurpos_ = false;
	    return false;
	}
	prevpos_ = pos;

	calcurpos_ = desiredvolume.hrg.includes( pos );
	if ( !calcurpos_ ) return false;
    }

    return true;
}


class COLineKeyProvider : public LineKeyProvider
{
public:

COLineKeyProvider( SeisTrcStorOutput& c, const char* a, const char* lk )
	: co(c) , attrnm(a) , linename(lk) {}

LineKey lineKey() const
{
    LineKey lk(linename,attrnm);
    return lk;
}
    SeisTrcStorOutput&   co;
    BufferString        attrnm;
    BufferString 	linename;

};


void SeisTrcStorOutput::collectData( const DataHolder& data, float refstep, 
				     const SeisTrcInfo& info, int outidx )
{
    if ( !calcurpos_ ) return;

    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs.size())
	return;

    const int sz = data.nrsamples_;
    DataCharacteristics dc;
    SeisTrc trc( sz, dc );
    for ( int idx=trc.data().nrComponents(); idx<desoutputs.size(); idx++)
	trc.data().addComponent( sz, dc, false );

    trc.info() = info;
    trc.info().sampling.step = refstep;
    trc.info().sampling.start = data.z0_*refstep;

    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.series(desoutputs[comp])->value(idx);
	    trc.set(idx, val, comp);
	}
    }

    if ( !storinited_ )
    {
	if ( writer_->is2D() )
	{
	    BufferString nm = curLineKey().attrName();
	    if ( nm == "inl_dip" || nm == "crl_dip" )
		nm = sKey::Steering;
	    else if ( IOObj::isKey(nm) )
		nm = IOM().nameOf(nm);
	    writer_->setLineKeyProvider( 
		new COLineKeyProvider( *this, nm, curLineKey().lineName()) );
	}

	if ( !writer_->prepareWork(trc) )
	    { errmsg = writer_->errMsg(); return; }

	if ( !writer_->is2D() )
	{
	    SeisTrcTranslator* transl = writer_->translator();
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& cis
		             = transl->componentInfo();
	    cis[0]->datatype = Seis::UnknowData;
	}

	storinited_ = true;
    }
    
    if ( !writer_->put(trc) )
	{ errmsg = writer_->errMsg(); }
}


TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			LineKey lkey )
    : errmsg(0)
    , datahset_(0)
    , trcinfoset_(0)
{
    seldata_.linekey_ = lkey;
    setGeometry( trg, zrg );

    const float dz = SI().zStep();
    Interval<int> interval( mNINT(seldata_.zrg_.start/dz), 
	    		    mNINT(seldata_.zrg_.stop/dz) );
    sampleinterval_ += interval;
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_.crlrg_.includes(bid.crl);
} 
 

void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    seldata_.zrg_ = zrg;
    assign( seldata_.crlrg_, trg );
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
			      const SeisTrcInfo& info, int outidx )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs.size() )
	return;

    if ( !datahset_ || !trcinfoset_ ) return;

    (*datahset_) += data.clone();

    SeisTrcInfo* trcinfo = new SeisTrcInfo(info);
    trcinfo->sampling.step = refstep;
    trcinfo->sampling.start = data.z0_*refstep;
    (*trcinfoset_) += trcinfo;
}


void TwoDOutput::setOutput( ObjectSet<DataHolder>& dataset,
			    ObjectSet<SeisTrcInfo>& trcinfoset )
{
    datahset_ = &dataset;
    trcinfoset_ = &trcinfoset;
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( true );
    seldata_.table_ = bidvalset;
}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info, int outidx )
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( info.binid );
    if ( !pos.valid() ) return;

    const int desnrvals = outidx+desoutputs.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	const int zidx = mNINT(vals[0]/refstep);
	if ( data.z0_ == zidx )
	{
	    for ( int comp=0; comp<desoutputs.size(); comp++ )
		vals[outidx+comp+1] = data.series(desoutputs[comp])->value(0);
	}

	bidvalset_.next( pos );
	if ( info.binid != bidvalset_.getBinID(pos) )
	    break;
    }
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRange(const BinID& bid) const
{
    const float dz = SI().zStep();
    TypeSet< Interval<int> > sampleinterval;

    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    while ( pos.valid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	Interval<int> interval( mNINT(vals[0]/dz), mNINT(vals[0]/dz) );
	sampleinterval += interval;
	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
	    break;
    }

    return sampleinterval;
}


TrcSelectionOutput::TrcSelectionOutput( const BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
{
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
				      const SeisTrcInfo& info, int outidx )
{
    const int nrcomp = data.nrSeries();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs.size() )
	return;

    const int trcsz = mNINT(stdtrcsz_/refstep) + 1;
    const int startidx = data.z0_ - mNINT(stdstarttime_/refstep);
    const int index = outpbuf_->find( info.binid );

    SeisTrc* trc;
    if ( index == -1 )
    {
	const DataCharacteristics dc;
	trc = new SeisTrc( trcsz, dc );
	for ( int idx=trc->data().nrComponents(); idx<desoutputs.size(); idx++ )
	    trc->data().addComponent( trcsz, dc, false );

	trc->info() = info;
	trc->info().sampling.start = stdstarttime_;
	trc->info().sampling.step = refstep;
    }
    else
	trc = outpbuf_->get( index );

    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc->set( idx, mUdf(float), comp );
	    else  
	    {
		const float val = 
		    data.series(desoutputs[comp])->value(idx-startidx);
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


TypeSet< Interval<int> > 
	TrcSelectionOutput::getLocalZRange( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    BinID binid;
    const float dz = SI().zStep();
    TypeSet<float> values;
    bidvalset_.get( pos, binid, values );
    TypeSet< Interval<int> > sampleinterval;
    for ( int idx=0; idx<values.size()/2; idx+=2 )
    {
	Interval<int> interval( mNINT(values[idx]/dz), 
				mNINT(values[idx+1]/dz) );
	sampleinterval += interval;
    }
 
    return sampleinterval;
}

} // namespace Attrib
