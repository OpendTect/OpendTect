/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.12 2005-08-04 14:27:41 cvshelene Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisread.h"
#include "seisreq.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "attriblinebuffer.h"
#include "threadwork.h"
#include "basictask.h"

namespace Attrib
{


void StorageProvider::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new SeisStorageRefParam(keyStr()) );

    if ( !desc->init() )
    { desc->unRef(); return; }

    desc->addOutputDataType( Seis::UnknowData );

    PF().addDesc( desc, createFunc );
    desc->unRef();
}


Provider* StorageProvider::createFunc( Desc& ds )
{
    StorageProvider* res = new StorageProvider( ds );
    res->ref();

    if ( !res->isOK() ) { res->unRef(); return 0; }

    res->unRefNoDelete();
    return res; 
}


void StorageProvider::updateDesc( Desc& ds )
{
    ds.removeOutputs();

    const LineKey lk( ((ValParam*)ds.getParam(keyStr()))->getStringValue(0) );

    const MultiID key( lk.lineName() );
    const BufferString attrnm = lk.attrName();
    
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork() ) return;

    SeisTrcTranslator* transl = rdr.translator();
    Seis2DLineSet* lset = rdr.lineSet();

    const bool is2d = rdr.is2D();
    if ( (is2d && !lset) || (!is2d && !transl) ) return;

    if ( is2d )
    {
	const bool issteering = attrnm==sKey::Steering;
	if ( issteering )
	{
	    ds.addOutputDataType( Seis::Dip );
	    ds.addOutputDataType( Seis::Dip );
	}
	else
	{
	    ds.addOutputDataType( Seis::UnknowData );
	}
    }
    else
    {
	const int nrattribs = transl->componentInfo().size();

	for ( int idx=0; idx<nrattribs; idx++ )
	    ds.addOutputDataType( (Seis::DataType)
		    transl->componentInfo()[idx]->datatype );
    }
}


StorageProvider::StorageProvider( Desc& desc_ )
    : Provider( desc_ )
    , status( Nada )
    , currentreq( 0 )
{
}


StorageProvider::~StorageProvider()
{
    deepErase( rg );
}


bool StorageProvider::init()
{
    if ( status!=Nada ) return false;

    const LineKey lk( ((ValParam*)desc.getParam(keyStr()))->getStringValue(0) );
    const MultiID mid( lk.lineName() );

    IOPar iopar;
    iopar.set( IOPar::compKey( SeisRequester::sKeysubsel, "ID" ) , mid );
    rg.usePar( iopar );
    if ( !rg.size() ) return false;

    bool isset = false;
    for ( int req=0; req<rg.size(); req++ )
    {
	if ( !initSeisRequester(req) ) { deepErase( rg ); return false; }

	const SeisTrcReader* reader = rg[req]->reader();
	if ( !reader ) { deepErase(rg); return false; }

	if ( reader->is2D() )
	{
	    const Seis2DLineSet* lset = reader->lineSet();
	    if ( !lset ) { deepErase(rg); return false; }

	    const int lineidx = lset->indexOf( lk.buf() );
	    if ( lineidx==-1 )
	    {
		storedvolume.hrg.start.inl = 0;
		storedvolume.hrg.stop.inl = lset->nrLines()-1;
		storedvolume.hrg.step.inl = 1;

		for ( int idx=0; idx<lset->nrLines(); idx++ )
		{
		    StepInterval<int> trcrg;
		    StepInterval<float> zrg;
		    if ( !lset->getRanges( idx, trcrg, zrg ) )
			continue;

		    if ( !isset )
		    {
			isset = true;
			storedvolume.hrg.start.crl = trcrg.start;
			storedvolume.hrg.stop.crl = trcrg.stop;
			storedvolume.zrg.start = zrg.start;
			storedvolume.zrg.stop = zrg.stop;
		    }
		    else
		    {
			storedvolume.hrg.start.crl =
			    mMIN(trcrg.start, storedvolume.hrg.start.crl );
			storedvolume.hrg.stop.crl =
			    mMAX(trcrg.stop, storedvolume.hrg.stop.crl );

			storedvolume.zrg.include( zrg );
		    }
		}
	    }
	    else
	    {
		storedvolume.hrg.start.inl = lineidx;
		storedvolume.hrg.stop.inl = lineidx;
		StepInterval<int> trcrg;
		StepInterval<float> zrg;
		if ( lset->getRanges( lineidx, trcrg, zrg ) )
		{
		    isset = true;
		    storedvolume.hrg.start.crl = trcrg.start;
		    storedvolume.hrg.stop.crl = trcrg.stop;
		    storedvolume.zrg.start = zrg.start;
		    storedvolume.zrg.stop = zrg.stop;
		}
	    }
	}
	else
	{
	    SeisTrcTranslator::getRanges(mid,storedvolume,lk);
	    isset = true;
	}
    }

    if ( !isset ) { deepErase( rg ); return false; }

    status = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace()
{
    if ( alreadymoved )
	return 1;

    if ( status==Nada )
	return -1;

    if ( status==StorageOpened )
    {
	if ( !setSeisRequesterSelection( currentreq ) )
	    return -1;

	status = Ready;
    }

    while ( true ) 
    {
	const int res = rg[currentreq]->next();
	if ( res==-1 ) return -1;
	if ( !res )
	{
	    currentreq++;
	    if ( currentreq>=rg.size() )
		return 0;

	    if ( !setSeisRequesterSelection(currentreq) )
		return -1;

	    continue;
	}
	if ( res<3 )
	{
	    SeisTrc* trc = rg[currentreq]->get(0,0);
	    if ( trc )
	    {
		currentbid = trc->info().binid;
		if ( strcmp(curlinekey_.lineName(),"") )
		    trcnr_ = trc->info().nr;
		
		break;
	    }
	}
    }

    alreadymoved = true;
    return 1;
}


bool StorageProvider::getPossibleVolume( int, CubeSampling& res )
{
    res = storedvolume;
    if ( !possiblevolume ) possiblevolume = new CubeSampling;
    *possiblevolume = storedvolume;
    return true;
}


SeisRequester* StorageProvider::getSeisRequester()
{ return currentreq!=-1 && currentreq<rg.size() ? rg[currentreq] : 0; }


bool StorageProvider::initSeisRequester(int req)
{
    rg[req]->setStepout( bufferstepout.inl, bufferstepout.crl );
    return rg[req]->prepareWork();
}


void StorageProvider::setBufferStepout( const BinID& ns )
{
    if ( ns.inl <= bufferstepout.inl && ns.crl <= bufferstepout.crl )
	return;

    if ( ns.inl > bufferstepout.inl ) bufferstepout.inl = ns.inl;
    if ( ns.crl > bufferstepout.crl ) bufferstepout.crl = ns.crl;

    updateStorageReqs();
}


void StorageProvider::updateStorageReqs(bool)
{
    for ( int req=0; req<rg.size(); req++ )
	initSeisRequester(req);
}


bool StorageProvider::setSeisRequesterSelection(int req)
{
    SeisTrcReader* reader = rg[req]->reader();
    if ( !reader ) return false;

    if ( seldata_.type_ == Seis::Table )
    {
	reader->setSelData( new SeisSelData(seldata_) ); //Memleak!
    }
    else if ( seldata_.type_ == Seis::Range )
    {
	if ( !desiredvolume ) return true;
	if ( ! &storedvolume ) return false;
	
	if ( reader->is2D() )
	{
	    Seis2DLineSet* lset = reader->lineSet();
	    if ( !lset ) return false;
	    seldata_.linekey_.setAttrName( curlinekey_.attrName() );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    if ( (const char*)curlinekey_.lineName() != "" )
	    {
		seldata_.linekey_.setLineName( curlinekey_.lineName() );
		int idx = lset->indexOf( curlinekey_ );
		if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
		{
		    seldata_.crlrg_.start = 
				desiredvolume->hrg.start.crl < trcrg.start?
				trcrg.start : desiredvolume->hrg.start.crl;
		    seldata_.crlrg_.stop = 
				desiredvolume->hrg.stop.crl > trcrg.start ?
				trcrg.stop : desiredvolume->hrg.stop.crl;
		    seldata_.zrg_.start = 
				desiredvolume->zrg.start < zrg.start ?
				zrg.start : desiredvolume->zrg.start;
		    seldata_.zrg_.stop = 
				desiredvolume->zrg.stop > zrg.stop ?
				zrg.stop : desiredvolume->zrg.stop;
		}
		reader->setSelData( new SeisSelData(seldata_) );
	    }

	    return true;
	}
	
	CubeSampling cs;
	cs.hrg.start.inl = 
	    	desiredvolume->hrg.start.inl < storedvolume.hrg.start.inl ?
		storedvolume.hrg.start.inl : desiredvolume->hrg.start.inl;
	cs.hrg.stop.inl = 
	    	desiredvolume->hrg.stop.inl > storedvolume.hrg.stop.inl ?
		storedvolume.hrg.stop.inl : desiredvolume->hrg.stop.inl;
	cs.hrg.stop.crl = 
	    	desiredvolume->hrg.stop.crl > storedvolume.hrg.stop.crl ?
		storedvolume.hrg.stop.crl : desiredvolume->hrg.stop.crl;
	cs.hrg.start.crl = 
	    	desiredvolume->hrg.start.crl < storedvolume.hrg.start.crl ?
	    	storedvolume.hrg.start.crl : desiredvolume->hrg.start.crl;
	cs.zrg.start = desiredvolume->zrg.start < storedvolume.zrg.start ?
		    	storedvolume.zrg.start : desiredvolume->zrg.start;
	cs.zrg.stop = desiredvolume->zrg.stop > storedvolume.zrg.stop ?
			 storedvolume.zrg.stop : desiredvolume->zrg.stop;

	seldata_.inlrg_.start = cs.hrg.start.inl;
	seldata_.inlrg_.stop = cs.hrg.stop.inl;
	seldata_.crlrg_.start = cs.hrg.start.crl;
	seldata_.crlrg_.stop = cs.hrg.stop.crl;
	seldata_.zrg_.start = cs.zrg.start;
	seldata_.zrg_.stop = cs.zrg.stop;

	reader->setSelData( new SeisSelData(seldata_) );

	SeisTrcTranslator* transl = reader->translator();

	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( !outputinterest[idx] ) 
		transl->componentInfo()[idx]->destidx = -1;
	}
    }

    return true;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int t0, int nrsamples ) const
{
    BinID nullbid(0,0);
    SeisTrc* trc;
    if ( relpos==nullbid )
	trc = rg[currentreq]->get(0,0);
    else
    {
	if (trcnr_ == -1)
	    trc = rg[currentreq]->get(currentbid + relpos);
	else
	{
	    BinID bid(currentbid);
	    bid.crl = trcnr_;
	    trc = rg[currentreq]->get(bid + relpos);
	}
    }
    
    if ( trc )
    {
	fillDataHolderWithTrc( trc, output );
	return true;
    }
    else
	return false;
}


void StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc, 
				    const DataHolder& trcsamplvalues ) const
{
    int t0 = trcsamplvalues.t0_;
    float starttime = storedvolume.zrg.start;
    float trcstep = trc->info().sampling.step;
    float reqstarttime = starttime<0 ? t0 * trcstep + starttime : t0 * trcstep;
    for (int idx=0 ; idx<trcsamplvalues.nrsamples_ ; idx++)
    {
	for ( int idy=0; idy< outputinterest.size(); idy++ )
	    if ( outputinterest[idy] )
	    {
		float val = trc->getValue( reqstarttime + idx*trcstep, idy );
		trcsamplvalues.item(idy)->setValue(idx,val);
	    }
    }
}


BinID StorageProvider::getStepoutStep(bool& found)
{
    SeisRequester* req = getSeisRequester();
    if ( !req ) return (0,0);
    SeisTrcTranslator* transl = req->reader()->translator();
    BinID mystep( transl->packetInfo().inlrg.step,
		  transl->packetInfo().crlrg.step );
    found = true;
    return mystep;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    const SeisTrcReader* reader = rg[currentreq]->reader();
    if ( reader->is2D() )
    {
        const Seis2DLineSet* lset = reader->lineSet();
	int idx = lset->indexOf( curlinekey_ );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
	{
	    storedvolume.hrg.start.crl = trcrg.start;
	    storedvolume.hrg.stop.crl = trcrg.stop;
	    storedvolume.zrg.start = zrg.start;
	    storedvolume.zrg.stop = zrg.stop;
	}
    }
}

}; //namespace
