/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.5 2005-05-31 12:50:09 cvshelene Exp $";

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
    : desiredvolume( cs )
    , sliceset( 0 )
{
    const float dz = SI().zRange().step;
    sampleinterval.start = (int)( cs.zrg.start / dz );
    const float stop = ( cs.zrg.stop / dz );
    sampleinterval.stop = (int)stop;
    if ( stop-sampleinterval.stop )
	sampleinterval.stop++;
}


SliceSetOutput::~SliceSetOutput() { if ( sliceset ) sliceset->unRef(); }


bool SliceSetOutput::getDesiredVolume(CubeSampling& cs) const
{ cs=desiredvolume; return true; }


bool SliceSetOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


Interval<int> SliceSetOutput::getLocalZRange(const BinID&) const
{ return sampleinterval; }


void SliceSetOutput::collectData( const BinID& bid, 
				  const DataHolder& data, float refstep, int )
{
    if ( !sliceset )
    {
	sliceset = new Attrib::SliceSet;
	sliceset->sampling = desiredvolume;
	sliceset->sampling.zrg.step = refstep;
	sliceset->direction = sliceset->defaultDirection(desiredvolume);
#define mGetDim(nr) \
        const int dim##nr = sliceset->dim( nr, sliceset->direction )
	mGetDim(0); mGetDim(1); mGetDim(2);
	for ( int idx=0; idx<dim0; idx++ )
	    *sliceset += new Attrib::Slice( dim1, dim2, mUndefValue );
    }

    int i0, i1, i2;
    for ( int idx=0; idx<sliceset->sampling.nrZ(); idx++)
    {
	float val = ( idx >= data.t0_ && idx < data.t0_+data.nrsamples_ ) ?
	     		data.item(0)->value(idx-data.t0_): mUndefValue;
	sliceset->getIdxs( bid.inl, bid.crl, idx*refstep, i0, i1, i2 );
	((*sliceset)[i0])->set( i1, i2, val );
    }
}


SliceSet* SliceSetOutput::getSliceSet() const
{
    return sliceset;
}


CubeOutput::CubeOutput( const CubeSampling& cs , LineKey lkey)
    : desiredvolume( cs )
    , auxpars(0)
    , storid_(*new MultiID)
    , writer_(0)
    , calcurpos_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , lkey_(lkey)
    , errmsg(0)
{
    const float dz = SI().zRange().step;
    sampleinterval.start = (int)( cs.zrg.start / dz );
    const float stop = ( cs.zrg.stop / dz );
    sampleinterval.stop = (int)stop;
    if ( stop-sampleinterval.stop )
	sampleinterval.stop++;

    seldata_.linekey_ = lkey;
}


bool CubeOutput::getDesiredVolume(CubeSampling& cs) const
{ cs=desiredvolume; return true; }


bool CubeOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


Interval<int> CubeOutput::getLocalZRange(const BinID&) const
{ return sampleinterval; }


//usePar, updTrc, init?


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
	PtrMan<IOObj> ioseisout = IOM().get( (const char*)storid_ );
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
    if ( seldata_.type_ != SeisSelData::Range )
	seldata_.type_ = SeisSelData::Range;

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

/*//use that when we get the refstep to resize trc if needed.//def trcids
    for ( int icomp=0; icomp<trcids.size(); icomp++ )
    {
	comptrc.data().reSize( nrsamps, icomp );
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	comptrc.set( isamp, udfval, icomp );
    }*/

    return true;
}

//nrVals a implementer.
//Trouver d'ou incrementer le trcattribs (qqpart in AttribEngMan)
//setReqs? I don't really see the need of that now...
//-> ensure twice that everything is ok

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

    const int nrattribs = desoutputs.size();
    for ( int attrib=0; attrib<nrattribs; attrib++ )
    {
//	if ( !processor.setCurrentOutpInterval( seldata_.zrg_, trcids[attrib],
//						desoutputs[attrib] ) )
	    //TODO fonction a remplacer, pas de ref au processor ici.
//	{
//	    calcurpos_ = false;
//	    errmsg = processor.errMsg();
//	    return false;
//	}
    }

    return true;
}

SeisTrcBuf* CubeOutput::getTrcBuf()
{
    SeisTrcBuf* ret = buf2d_;
    buf2d_ = 0;
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
			    float refstep, int trcnr )
{
    if ( !calcurpos_ ) return;

    ObjectSet<const SeisTrc> trcs;//TODO consider the case of multiple providers
    //leading to multiple trcs: what is then the dataHolder?
    //here dataholder only have results for the selected outputs of one single 
    //provider..., so if nrtrcs>1 do a loop over nrtrcs 
    //(look in old CubeAttribOutput::createOutput).
    int nrcomp = data.nrItems();
    if ( !nrcomp || nrcomp < desoutputs.size())
	return;

    int sz = data.nrsamples_;
    const DataCharacteristics dc;
    SeisTrc* trc = new SeisTrc(sz, dc);
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
	    BufferString  nm = curLineKey().attrName();
	    if ( nm == "inl_dip"
		  || nm == "crl_dip" )
		nm = sKey::Steering;
	    else if ( IOObj::isKey(nm) )
		nm = IOM().nameOf(nm);
	    writer_->setLineKeyProvider( 
		    new COLineKeyProvider(*this, nm, curLineKey().lineName()) );
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

	//transl->packetInfo().stdinfo = really important?
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
	    buf2d_->add( new SeisTrc( *trc ) );
    }

    // TODO later on : create function on writer to handle dataholder directly
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    seldata_.type_ = SeisSelData::Table;
    seldata_.table_.allowDuplicateBids( true );
    seldata_.table_ = bidvalset;
}


void LocationOutput::collectData( const BinID& bid,const DataHolder& data,
                             float refstep, int trcnr )
{
    int ii = bid.inl; int jj = bid.crl;
    BinIDValueSet::Pos pos( ii, jj );
    TypeSet<float> collectval;
    float* vals = bidvalset_.getVals( pos );
    collectval += *(vals); 
    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	float val = data.item(desoutputs[comp])->value(0);
	collectval += val;
    }
    bidvalset_.set(pos,collectval);
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    if ( pos.i == 0 || pos.j == 0 )
	return false;

    return true;
}


Interval<int> LocationOutput::getLocalZRange(const BinID& bid) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    const float* vals = bidvalset_.getVals(pos);
    const_cast<LocationOutput*>(this)->sampleinterval.start =(int)vals[0];
    const_cast<LocationOutput*>(this)->sampleinterval.stop = (int)vals[0];
    return sampleinterval;
}


TrcSelectionOutput::TrcSelectionOutput( const char* blablaclass )
//    : bla_(blabla)
{
    seldata_.type_ = SeisSelData::Table;
    seldata_.table_.allowDuplicateBids( true );
//    seldata_.table_ = ;
}


void TrcSelectionOutput::collectData( const BinID& bid,const DataHolder& data,
                             float refstep, int trcnr )
{
    //add code
    for ( int comp=0; comp<desoutputs.size(); comp++ )
    {
	float val = data.item(desoutputs[comp])->value(0);
    }
}


bool TrcSelectionOutput::wantsOutput( const BinID& bid ) const
{
    //make specific code
    return true;
}


Interval<int> TrcSelectionOutput::getLocalZRange(const BinID& bid) const
{
    //make specific code
    return sampleinterval;
}

}; //namespace
