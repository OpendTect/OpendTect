/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attriboutput.cc,v 1.2 2005-05-09 14:40:41 cvshelene Exp $";

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
{}


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
	float val = ( idx >= data.t1_ && idx < data.t1_+data.nrsamples_ ) ?
	     		data.item(0)->value(idx-data.t1_): mUndefValue;
	sliceset->getIdxs( bid.inl, bid.crl, idx*refstep, i0, i1, i2 );
	((*sliceset)[i0])->set( i1, i2, val );
    }
}


SliceSet* SliceSetOutput::getSliceSet() const
{
    return sliceset;
}


StorageOutput::StorageOutput( const CubeSampling& cs , const char* lkstr)
    : desiredvolume( cs )
    , seldata(*new SeisSelData)
    , auxpars(0)
    , storid_(*new MultiID)
    , errmsg(0)
{
    const float dz = SI().zRange().step;
    sampleinterval.start = (int)( cs.zrg.start / dz );
    const float stop = ( cs.zrg.stop / dz );
    sampleinterval.stop = (int)stop;
    if ( stop-sampleinterval.stop )
	sampleinterval.stop++;

    seldata.linekey_ = lkstr;
}


StorageOutput::~StorageOutput() 
{
    delete &seldata;
    delete &storid_;
    delete auxpars;
}


bool StorageOutput::getDesiredVolume(CubeSampling& cs) const
{ cs=desiredvolume; return true; }


bool StorageOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume.hrg.includes(bid); }


Interval<int> StorageOutput::getLocalZRange(const BinID&) const
{ return sampleinterval; }


//usePar, updTrc, init?


bool StorageOutput::setStorageID( const MultiID& storid )
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


void StorageOutput::setGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;
    seldata.copyFrom(cs);
}


CubeOutput::CubeOutput( const CubeSampling& cs, const char* lk )
        : writer_(0)
    	, calcurpos_(0)
    	, prevpos_(-1,-1)
        , StorageOutput(cs,lk)
    	, storinited_(0)
    	,lk_(lk)
{
}


CubeOutput::~CubeOutput()
{
    delete writer_;
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
    seldata.linekey_.setAttrName( "" );
    if ( seldata.type_ != SeisSelData::Range )
	seldata.type_ = SeisSelData::Range;

    if ( !is2d_ )
    {
	if ( seldata.inlrg_.start > desiredvolume.hrg.start.inl )
	    desiredvolume.hrg.start.inl = seldata.inlrg_.start;
	if ( seldata.inlrg_.stop < desiredvolume.hrg.stop.inl )
	    desiredvolume.hrg.stop.inl = seldata.inlrg_.stop;
	if ( seldata.crlrg_.start > desiredvolume.hrg.start.crl )
	    desiredvolume.hrg.start.crl = seldata.crlrg_.start;
	if ( seldata.crlrg_.stop < desiredvolume.hrg.stop.crl )
	    desiredvolume.hrg.stop.crl = seldata.crlrg_.stop;
	if ( seldata.zrg_.start > desiredvolume.zrg.start )
	    desiredvolume.zrg.start = seldata.zrg_.start;
	if ( seldata.zrg_.stop < desiredvolume.zrg.stop )
	    desiredvolume.zrg.stop = seldata.zrg_.stop;
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
//	if ( !processor.setCurrentOutpInterval( seldata.zrg_, trcids[attrib],
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
    LineKey lk(linename);
    return lk;
}
    CubeOutput&   co;
    BufferString        attrnm;
    const char* 	linename;

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
    if ( !nrcomp || nrcomp != desoutputs.size())
	return;

    int sz = data.nrsamples_;
    const DataCharacteristics dc;
    SeisTrc* trc = new SeisTrc(sz, dc);
    for ( int idx=trc->data().nrComponents(); idx<nrcomp; idx++)
	trc->data().addComponent( sz, dc, false );

    trc->info().sampling.step = refstep;
    trc->info().sampling.start = data.t1_*refstep;
    trc->info().binid = bid;
    if ( is2d_ )
	trc->info().nr = trcnr;

    for ( int comp=0; comp<nrcomp; comp++ )
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
	    BufferString  nm = seldata.linekey_.attrName();
	    if ( nm == "inl_dip"
		  || nm == "crl_dip" )
		nm = sKey::Steering;
	    else if ( IOObj::isKey(nm) )
		nm = IOM().nameOf(nm);
	//    else if ( isstoredcube_ )//????
	//	nm = LineKey(nm).attrName();
	    writer_->setLineKeyProvider( 
		    new COLineKeyProvider(*this,nm, curLineName()) );
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

}; //namespace
