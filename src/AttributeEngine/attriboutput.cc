/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.20 2005-08-18 14:19:21 cvsnanne Exp $";

#include "attriboutput.h"
#include "survinfo.h"
#include "attribslice.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "iopar.h"
#include "seiswrite.h"
#include "ioman.h"
#include "ptrman.h"
#include "ioobj.h"
#include "linekey.h"


namespace Attrib
{

const char* Output::outputstr = "Output";
const char* Output::typekey = "Type";
const char* Output::cubekey = "Cube";
const char* Output::surfkey = "Surface";
const char* Output::tskey = "Trace Selection";
const char* Output::scalekey = "Scale";

const char* CubeOutput::seisidkey = "Seismic ID";
const char* CubeOutput::attribkey = "Attributes";
const char* CubeOutput::inlrangekey = "In-line range";
const char* CubeOutput::crlrangekey = "Cross-line range";
const char* CubeOutput::depthrangekey = "Depth range";

const char* LocationOutput::filenamekey = "Output Filename";
const char* LocationOutput::locationkey = "Locations";
const char* LocationOutput::attribkey = "Attribute";
const char* LocationOutput::surfidkey = "Surface ID";


Output::~Output()
{
    delete &seldata_;
}


Output::Output()
    : seldata_(*new SeisSelData)
{
    mRefCountConstructor;
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


void SliceSetOutput::collectData( const BinID& bid, const DataHolder& data,
				  float refstep, int trcnr, int outidx )
{
    if ( !sliceset )
    {
	sliceset = new Attrib::SliceSet;
	sliceset->ref();
	sliceset->sampling = desiredvolume;
	sliceset->sampling.zrg.step = refstep; 
	sliceset->direction = desiredvolume.defaultDir();
#define mGetDim(nr) \
        const int dim##nr = \
	    desiredvolume.size( direction(sliceset->direction,nr) )
		
	mGetDim(0); mGetDim(1); mGetDim(2);
	for ( int idx=0; idx<dim0; idx++ )
	    *sliceset += new Attrib::Slice( dim1, dim2, udfval );

	if ( dim0 == 1 && desoutputs.size() > 1 )
	    for ( int idx=0; idx<desoutputs.size()-1; idx++ )
		*sliceset += new Attrib::Slice( dim1, dim2, udfval );
    }

    if ( !sliceset->sampling.hrg.includes(bid) )
	return;

    int i0, i1, i2;
    int sampleoffset = mNINT(sliceset->sampling.zrg.start/refstep);
    for ( int idy=0; idy<desoutputs.size(); idy++ )
    {
	for ( int idx=sampleoffset; 
		idx<sliceset->sampling.nrZ()+sampleoffset; idx++)
	{
	    const bool valididx = idx>=data.t0_ && idx<data.t0_+data.nrsamples_;
	    const float val = valididx ? 
			data.item(desoutputs[idy])->value(idx-data.t0_): udfval;
	    sliceset->getIdxs( bid.inl, bid.crl, idx*refstep, i0, i1, i2 );
	    ((*sliceset)[i0+idy])->set( i1, i2, val );
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


CubeOutput::CubeOutput( const CubeSampling& cs , LineKey lkey )
    : desiredvolume(cs)
    , auxpars(0)
    , storid_(*new MultiID)
    , writer_(0)
    , calcurpos_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , lkey_(lkey)
    , buf2d_(0)
    , errmsg(0)
{
    const float dz = SI().zStep();
    Interval<int> interval( mNINT(cs.zrg.start/dz), mNINT(cs.zrg.stop/dz) );
    sampleinterval += interval;

    seldata_.linekey_ = lkey;
}


bool CubeOutput::getDesiredVolume( CubeSampling& cs ) const
{ cs=desiredvolume; return true; }


bool CubeOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


TypeSet< Interval<int> > CubeOutput::getLocalZRange( const BinID& ) const
{ return sampleinterval; }


bool CubeOutput::setStorageID( const MultiID& storid )
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


void CubeOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata_.copyFrom(cs);
}


CubeOutput::~CubeOutput()
{
    delete writer_;
    delete &storid_;
    delete auxpars;
    if ( buf2d_ )
	buf2d_->erase();
}


bool CubeOutput::doUsePar( const IOPar& pars )
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


bool CubeOutput::doInit()
{
    if ( *((const char*)storid_) )
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


bool CubeOutput::setReqs( const BinID& pos )
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


SeisTrcBuf* CubeOutput::getTrcBuf() const 
{
    SeisTrcBuf* ret = buf2d_;
    const_cast<CubeOutput*>(this)->buf2d_ = 0;
    return ret;
}


class COLineKeyProvider : public LineKeyProvider
{
public:

COLineKeyProvider( CubeOutput& c, const char* a, const char* lk )
	: co(c) , attrnm(a) , linename(lk) {}

LineKey lineKey() const
{
    LineKey lk(linename,attrnm);
    return lk;
}
    CubeOutput&   co;
    BufferString        attrnm;
    BufferString 	linename;

};


void CubeOutput::collectData( const BinID& bid, const DataHolder& data, 
			      float refstep, int trcnr, int outidx )
{
    if ( !calcurpos_ ) return;

    int nrcomp = data.nrItems();
    if ( !nrcomp || nrcomp < desoutputs.size())
	return;

    const int sz = data.nrsamples_;
    DataCharacteristics dc;
    SeisTrc* trc = new SeisTrc( sz, dc );
    for ( int idx=trc->data().nrComponents(); idx<desoutputs.size(); idx++)
	trc->data().addComponent( sz, dc, false );

    trc->info().sampling.step = refstep;
    trc->info().sampling.start = data.t0_*refstep;
    trc->info().binid = bid;
    trc->info().coord = SI().transform( bid );
    if ( is2d_ )
	trc->info().nr = trcnr;

    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.item(desoutputs[comp])->value(idx);
	    trc->set(idx, val, comp);
	}
    }

    const bool dostor = *((const char*)storid_);
    if ( dostor && !storinited_ )
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

	if ( !writer_->prepareWork(*trc) )
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
    
    if ( dostor )
    {
        if ( !writer_->put(*trc) )
            { errmsg = writer_->errMsg(); }
    }
    else if ( is2d_ )
    {
	if ( !buf2d_ ) buf2d_ = new SeisTrcBuf;
	buf2d_->add( new SeisTrc(*trc) );
    }

    // TODO later on : create function on writer to handle dataholder directly
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    seldata_.type_ = Seis::Table;
    seldata_.table_.allowDuplicateBids( true );
    seldata_.table_ = bidvalset;
}


void LocationOutput::collectData( const BinID& bid, const DataHolder& data,
				  float refstep, int trcnr, int outidx )
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    if ( !pos.valid() ) return;

    const int desnrvals = outidx+desoutputs.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	const int zidx = mNINT(vals[0]/refstep);
	if ( data.t0_ == zidx )
	{
	    for ( int comp=0; comp<desoutputs.size(); comp++ )
		vals[outidx+comp+1] = data.item(desoutputs[comp])->value(0);
	}

	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
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


void TrcSelectionOutput::collectData( const BinID& bid, const DataHolder& data,
				      float refstep, int trcnr, int outidx )
{
    int nrcomp = data.nrItems();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs.size() )
	return;

    const int sz = mNINT(stdtrcsz_/refstep);
    const int startidx = data.t0_ - mNINT(stdstarttime_/refstep);
    const int index = outpbuf_->find( bid );
    SeisTrc* trc;

    if ( index == -1 )
    {
	const DataCharacteristics dc;
	trc = new SeisTrc( sz, dc );
	for ( int idx=trc->data().nrComponents(); idx<desoutputs.size(); idx++ )
	    trc->data().addComponent( sz, dc, false );

	trc->info().sampling.step = refstep;
	trc->info().sampling.start = stdstarttime_;
	trc->info().binid = bid;
	trc->info().coord = SI().transform( bid );
    }
    else
	trc = outpbuf_->get( index );

    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc->set( idx, mUdf(float), comp );
	    else  
	    {
		const float val = 
		    data.item(desoutputs[comp])->value(idx-startidx);
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
