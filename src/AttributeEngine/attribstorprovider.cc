/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.47 2006-10-23 15:01:31 cvskris Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attriblinebuffer.h"
#include "attribdataholder.h"
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
#include "threadwork.h"
#include "basictask.h"

namespace Attrib
{


void StorageProvider::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new SeisStorageRefParam(keyStr()) );
    desc->addOutputDataType( Seis::UnknowData );

    PF().addDesc( desc, createFunc );
    desc->unRef();
}


Provider* StorageProvider::createFunc( Desc& desc )
{
    StorageProvider* res = new StorageProvider( desc );
    res->ref();

    if ( !res->isOK() ) { res->unRef(); return 0; }

    res->unRefNoDelete();
    return res; 
}


void StorageProvider::updateDesc( Desc& desc )
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );

    const MultiID key( lk.lineName() );
    const BufferString attrnm = lk.attrName();
    
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(Seis::PreScan)
      || rdr.psIOProv() ) return;

    if ( rdr.is2D() )
    {
	if ( !rdr.lineSet() ) return;
	const bool issteering = attrnm == sKey::Steering;
	if ( !issteering )
	{
	    SeisTrcTranslator* transl = rdr.seisTranslator();
	    if ( !transl || !transl->componentInfo().size() )
		desc.setNrOutputs( Seis::UnknowData, 1 );//why only 1 ?
	    else
		desc.setNrOutputs( (Seis::DataType)
				   transl->componentInfo()[0]->datatype, 1 );
	}
	else
	    desc.setNrOutputs( Seis::Dip, 2 );
    }
    else
    {
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl ) return;

	BufferString type;
	ioobj->pars().get( sKey::Type, type );

	const int nrattribs = transl->componentInfo().size();
	if ( type == sKey::Steering )
	    desc.setNrOutputs( Seis::Dip, nrattribs );
	else
	{
	    for ( int idx=1; idx<=nrattribs; idx++ )
		if ( desc.nrOutputs() < idx )
		    desc.addOutputDataType( (Seis::DataType)
				    transl->componentInfo()[idx-1]->datatype);
	}
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

    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
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

	desc.set2D( reader->is2D() );
	if ( reader->is2D() )
	{
	    const Seis2DLineSet* lset = reader->lineSet();
	    if ( !lset ) { deepErase(rg); return false; }

	    const int lineidx = lset->indexOf( lk.buf() );
	    if ( lineidx==-1 )
	    {
		storedvolume.hrg.start.inl = 0;
		storedvolume.hrg.stop.inl = 1;
		storedvolume.hrg.step.inl = 1;

		storedvolume.hrg.start.crl = 0;
		storedvolume.hrg.stop.crl = SI().maxNrTraces(true);
		storedvolume.hrg.step.crl = 1;

		storedvolume.zrg = SI().sampling(true).zrg;

		isset = true;
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
		    storedvolume.zrg.step = zrg.step;
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


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved )
	return 1;

    if ( status==Nada )
	return -1;

    if ( status==StorageOpened )
    {
	if ( !setSeisRequesterSelection(currentreq) )
	    return -1;

	status = Ready;
    }

    if ( getDesc().is2D() )
	prevtrcnr = currentbid.crl;

    bool validstartpos = startpos != BinID(-1,-1);
    if ( validstartpos && curtrcinfo_ && curtrcinfo_->binid == startpos )
    {
	alreadymoved = true;
	return 1;
    }

    bool cont = true;
    while ( cont )
    {
	int res = rg[currentreq]->next();
	switch ( res )
	{
	    case -1: return -1;
	    case 0:
	    {
		currentreq++;
		if ( currentreq>=rg.size() )
		    return 0;

		if ( !setSeisRequesterSelection(currentreq) )
		    return -1;

		continue;
	    }
	    case 1:
	    {
		SeisTrc* trc = rg[currentreq]->get(0,0);
		if ( trc )
		{
		    for ( int idx=0; idx<trc->nrComponents(); idx++ )
		    {
			if ( datachar_.size()<=idx )
			    datachar_ +=
				trc->data().getInterpreter()->dataChar();
			else
			    datachar_[idx] =
				trc->data().getInterpreter()->dataChar();
		    }

		    curtrcinfo_ = &trc->info();
		    currentbid = desc.is2D()? BinID( 0, curtrcinfo_->nr ) 
					    : curtrcinfo_->binid;
		    trcinfobid = curtrcinfo_->binid;
		    if ( !validstartpos || 
			 ( validstartpos && ( currentbid == startpos || 
			 ( curtrcinfo_->binid == startpos || firstcheck ) ) ))

			cont = false;
		    else
			curtrcinfo_ = 0;
		}

		continue;
	    }
	    case 2: case 3: continue;
	}
    }

    alreadymoved = true;
    return 1;
}


bool StorageProvider::getPossibleVolume( int, CubeSampling& res )
{
    if ( !possiblevolume ) 
	possiblevolume = new CubeSampling;
    
    *possiblevolume = storedvolume;
# define mAdjustIf(v1,op,v2) \
  if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;
  
    mAdjustIf(res.hrg.start.inl,<,possiblevolume->hrg.start.inl);
    mAdjustIf(res.hrg.start.crl,<,possiblevolume->hrg.start.crl);
    mAdjustIf(res.zrg.start,<,possiblevolume->zrg.start);
    mAdjustIf(res.hrg.stop.inl,>,possiblevolume->hrg.stop.inl);
    mAdjustIf(res.hrg.stop.crl,>,possiblevolume->hrg.stop.crl);
    mAdjustIf(res.zrg.stop,>,possiblevolume->zrg.stop);
    return true;
}


SeisRequester* StorageProvider::getSeisRequester() const
{
    return currentreq == -1 || currentreq >= rg.size()
	 ? 0 : const_cast<SeisRequester*>( rg[currentreq] );
}


bool StorageProvider::initSeisRequester( int req )
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


bool StorageProvider::setSeisRequesterSelection( int req )
{
    SeisTrcReader* reader = rg[req]->reader();
    if ( !reader || reader->psIOProv() ) return false;

    if ( seldata_ && seldata_->type_ == Seis::Table )
    {
	SeisSelData* seldata = new SeisSelData(*seldata_);
	seldata->extraz_ = extraz_;
	if ( reader->is2D() )
	{
	    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
	    seldata->linekey_.setAttrName( lk.attrName() );
	}
	reader->setSelData( seldata );
    }
    else if ( !seldata_ || (seldata_ && seldata_->type_ == Seis::Range) )
    {
	if ( !desiredvolume && !reader->is2D() ) 
	{
	    for ( int idp=0; idp<parents.size(); idp++ )
	    {
		desiredvolume = parents[idp]?parents[idp]->getDesiredVolume():0;
		if ( desiredvolume )
		{
		    if ( !checkDataOK() ) return false;
		    break;
		}
	    }
	    return true;
	}
	
	if ( ! &storedvolume ) return false;
	
	if ( reader->is2D() )
	{
	    SeisSelData* seldata;
	    if ( seldata_ )
		seldata = new SeisSelData(*seldata_);
	    else
		seldata = new SeisSelData(true);
	    Seis2DLineSet* lset = reader->lineSet();
	    if ( !lset ) return false;
	    seldata->linekey_.setAttrName( curlinekey_.attrName() );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    if ( (const char*)curlinekey_.lineName() != "" )
	    {
		seldata->linekey_.setLineName( curlinekey_.lineName() );
		int idx = lset->indexOf( curlinekey_ );
		if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
		{
		    if ( !checkDataOK( trcrg,zrg ) ) return false;
		    seldata->crlrg_.start = 
				desiredvolume->hrg.start.crl < trcrg.start?
				trcrg.start : desiredvolume->hrg.start.crl;
		    seldata->crlrg_.stop = 
				desiredvolume->hrg.stop.crl > trcrg.stop ?
				trcrg.stop : desiredvolume->hrg.stop.crl;
		    seldata->zrg_.start = 
				desiredvolume->zrg.start < zrg.start ?
				zrg.start : desiredvolume->zrg.start;
		    seldata->zrg_.stop = 
				desiredvolume->zrg.stop > zrg.stop ?
				zrg.stop : desiredvolume->zrg.stop;
		    seldata->inlrg_.stop = seldata->inlrg_.start = 0;
		}
		reader->setSelData( seldata );
	    }

	    return true;
	}

	if ( !checkDataOK() ) return false;
	
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

	SeisSelData* seldata;
	if ( seldata_ )
	    seldata = new SeisSelData(*seldata_);
	else
	    seldata = new SeisSelData(true);
	
	seldata->inlrg_.start = cs.hrg.start.inl;
	seldata->inlrg_.stop = cs.hrg.stop.inl;
	seldata->crlrg_.start = cs.hrg.start.crl;
	seldata->crlrg_.stop = cs.hrg.stop.crl;
	seldata->zrg_.start = cs.zrg.start;
	seldata->zrg_.stop = cs.zrg.stop;

	reader->setSelData( seldata );

	SeisTrcTranslator* transl = reader->seisTranslator();
	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( !outputinterest[idx] ) 
		transl->componentInfo()[idx]->destidx = -1;
	}
    }

    return true;
}


bool StorageProvider::checkDataOK()
{
    if ( desiredvolume->hrg.start.inl>storedvolume.hrg.stop.inl ||
	 desiredvolume->hrg.start.crl>storedvolume.hrg.stop.crl ||
	 desiredvolume->zrg.start>storedvolume.zrg.stop ||
	 desiredvolume->hrg.stop.inl<storedvolume.hrg.start.inl ||
	 desiredvolume->hrg.stop.crl<storedvolume.hrg.start.crl ||
	 desiredvolume->zrg.stop<storedvolume.zrg.start )
    {
	errmsg = "'"; errmsg += desc.userRef(); errmsg += "'"; 
	errmsg += " contains no data in selected area";
	return false;
    }

    return true;
}


bool StorageProvider::checkDataOK( StepInterval<int> trcrg, 
				   StepInterval<float>zrg )
{
    if ( desiredvolume->hrg.start.crl > trcrg.stop ||
	 desiredvolume->hrg.stop.crl < trcrg.start ||
	 desiredvolume->zrg.stop < zrg.start ||
	 desiredvolume->zrg.start > zrg.stop )
    {
	errmsg = "'"; errmsg += desc.userRef(); errmsg += "'"; 
	errmsg += " contains no data in selected area";
	return false;
    }

    return true;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples ) const
{
    BinID nullbid(0,0);
    const SeisTrc* trc;
    if ( relpos==nullbid )
	trc = rg[currentreq]->get(0,0);
    else
    {
	const BinID bid = desc.is2D() ? BinID(0,curtrcinfo_->nr) : currentbid;
	trc = rg[currentreq]->get( bid+relpos );
    }
    
    if ( !trc )
	return false;

    return fillDataHolderWithTrc( trc, output );
}


bool StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc, 
					     const DataHolder& data ) const
{
    const int z0 = data.z0_;
    int offset = 0;
    float exacttime = 0;
    if ( needinterp )
    {
	BinID curbid = desc.is2D() && curtrcinfo_ ?
		       curtrcinfo_->binid : currentbid;
	BinIDValueSet::Pos pos = seldata_->table_.findFirst( curbid );
	if ( !pos.valid() ) return false;

	exacttime = seldata_->table_.getVals( pos )[0];
	offset = mNINT( z0 - exacttime/refstep );
    }
    
    for ( int idx=0; idx<data.nrsamples_; idx++ )
    {
	const float curt = needinterp ? exacttime + (offset+idx)*refstep 
	    			      : (z0+idx)*refstep;
	for ( int idy=0; idy<outputinterest.size(); idy++ )
	{
	    if ( outputinterest[idy] )
	    {
		float val = trc->getValue( curt, idy );
		const_cast<DataHolder&>(data).series(idy)->setValue(idx,val);
	    }
	}
    }

    return true;
}


BinDataDesc StorageProvider::getOutputFormat( int output ) const
{
    if ( output>=datachar_.size() )
	return Provider::getOutputFormat( output );

    return datachar_[output];
}


BinID StorageProvider::getStepoutStep() const
{
    SeisRequester* req = getSeisRequester();
    if ( !req || !req->reader() || !req->reader()->seisTranslator() )
	return BinID(0,0);

    SeisPacketInfo& info = req->reader()->seisTranslator()->packetInfo();
    return BinID( info.inlrg.step, info.crlrg.step );
}


void StorageProvider::adjust2DLineStoredVolume( bool adjuststep )
{
    const SeisTrcReader* reader = rg[currentreq]->reader();
    if ( !reader->is2D() ) return;

    const Seis2DLineSet* lset = reader->lineSet();
    const int idx = lset->indexOf( curlinekey_ );
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
    {
	storedvolume.hrg.start.crl = trcrg.start;
	storedvolume.hrg.stop.crl = trcrg.stop;
	storedvolume.zrg.start = zrg.start;
	storedvolume.zrg.stop = zrg.stop;
	storedvolume.zrg.step = zrg.step;
    }

    if ( adjuststep )
	getZStepStoredData(refstep);
}

}; // namespace Attrib
