/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.111 2012/07/10 13:05:59 cvskris Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attriblinebuffer.h"
#include "attribdataholder.h"
#include "attribdatacubes.h"
#include "datainpspec.h"
#include "datapack.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "linesetposinfo.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisbounds.h"
#include "seisbufadapters.h"
#include "seiscubeprov.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seistrctr.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "threadwork.h"
#include "task.h"
#include <math.h>


namespace Attrib
{

mAttrDefCreateInstance(StorageProvider)

void StorageProvider::initClass()
{
    mAttrStartInitClassWithUpdate

    desc->addParam( new SeisStorageRefParam(keyStr()) );
    desc->addOutputDataType( Seis::UnknowData );

    mAttrEndInitClass
}


void StorageProvider::updateDesc( Desc& desc )
{
    updateDescAndGetCompNms( desc, 0 );
}


void StorageProvider::updateDescAndGetCompNms( Desc& desc,
					       BufferStringSet* compnms )
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );

    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( linenm && *linenm == '#' )
    {
	DataPack::FullID fid( linenm+1 );
	if ( !DPM(fid).haveID( fid ) )
	    desc.setErrMsg( "Cannot find data in memory" );

	return;
    }

    const MultiID key( lk.lineName() );
    const BufferString attrnm = lk.attrName();
    
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(Seis::PreScan) || rdr.psIOProv() )
    {
//	desc.setErrMsg( rdr.errMsg() );
	return;
    }

    if ( compnms )
	compnms->erase();

    if ( rdr.is2D() )
    {
	if ( !rdr.lineSet() )
	{
	    BufferString errmsg = "No line set available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg( errmsg );
	    return;
	}

	BufferStringSet steernms;
	rdr.lineSet()->getAvailableAttributes( steernms, sKey::Steering );
	const bool issteering = steernms.indexOf( attrnm ) >= 0;
	if ( !issteering )
	{
	    SeisTrcTranslator* transl = rdr.seisTranslator();
	    if ( !transl )
	       desc.setNrOutputs( Seis::UnknowData, 1 );//why only 1 ?
	    else if ( transl->componentInfo().isEmpty() )
	    {
		BufferStringSet complist;
		SeisIOObjInfo::getCompNames( lk, complist );
		desc.setNrOutputs( Seis::UnknowData, complist.size() );
		if ( compnms )
		    compnms->operator =( complist );
	    }
	    else
	    {
		for ( int idx=0; idx<transl->componentInfo().size(); idx++ )
		    desc.addOutputDataType( (Seis::DataType)
					transl->componentInfo()[0]->datatype );
	    } 
	}
	else
	{
	    desc.setNrOutputs( Seis::Dip, 2 );
	    if ( compnms )
		compnms->operator =( steernms );
	}
    }
    else
    {
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl )
	{
	    BufferString errmsg = "No data interpreter available for '";
	    errmsg += ioobj->name(); errmsg += "'";
//	    desc.setErrMsg ( errmsg );
	    return;
	}

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

	if ( compnms )
	    transl->getComponentNames( *compnms );
    }
}


StorageProvider::StorageProvider( Desc& desc )
    : Provider( desc )
    , mscprov_(0)
    , status_( Nada )
    , stepoutstep_(-1,0)
    , isondisc_(true)
{
    const LineKey lk( desc.getValParam(keyStr())->getStringValue(0) );
    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( linenm && *linenm == '#' )
    {
	DataPack::FullID fid( linenm+1 );
	isondisc_ =  !DPM(fid).haveID( fid );
    }
}


StorageProvider::~StorageProvider()
{
    if ( mscprov_ ) delete mscprov_;
}


#undef mErrRet
#define mErrRet(s) { errmsg_ = s; delete mscprov_; mscprov_= 0; return false; }

bool StorageProvider::checkInpAndParsAtStart()
{
    if ( status_!=Nada ) return false;

    if ( !isondisc_ ) return true;

    const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
    const MultiID mid( lk.lineName() );
    if ( !isOK() ) return false;
    mscprov_ = new SeisMSCProvider( mid );

    if ( !initMSCProvider() )
	mErrRet( mscprov_->errMsg() )

    const bool is2d = mscprov_->is2D();
    desc_.set2D( is2d );
    if ( !is2d )
	SeisTrcTranslator::getRanges( mid, storedvolume_, lk );
    else
    {
	Seis2DLineSet* lset = mscprov_->reader().lineSet();
	if ( !lset )
	    mErrRet( "2D seismic data/No line set found" );

	int lineidx = lset->indexOf( lk.buf() );
	if ( lineidx == -1 )
	{
	    storedvolume_.hrg.start.inl = 0;
	    storedvolume_.hrg.stop.inl = 1;
	    storedvolume_.hrg.include( BinID( 0,0 ) );
	    storedvolume_.hrg.include( BinID( 0,SI().maxNrTraces(true) ) );
	    storedvolume_.hrg.step.crl = 1; // what else?
	    BufferStringSet candidatelines;
	    lset->getLineNamesWithAttrib( candidatelines, lk.attrName() );
	    bool foundone = false;
	    for ( int idx=0; idx<candidatelines.size(); idx++ )
	    {
		LineKey tmplk( candidatelines.get(idx).buf(), lk.attrName() );
		lineidx = lset->indexOf( tmplk );
		if ( lineidx> -1 )
		{
		    StepInterval<int> trcrg; StepInterval<float> zrg;
		    if ( lset->getRanges( lineidx, trcrg, zrg ) )
		    {
			if ( foundone )
			{
			    storedvolume_.hrg.include( BinID(0,trcrg.start) );
			    storedvolume_.hrg.include( BinID(0,trcrg.stop) );
			    storedvolume_.zrg.include( zrg );
			}
			else
			{
			    storedvolume_.hrg.start.crl = trcrg.start;
			    storedvolume_.hrg.stop.crl = trcrg.stop;
			    storedvolume_.zrg = zrg;
			}
			foundone = true;
		    }
		}
	    }
	}
	else
	{
	    storedvolume_.hrg.start.inl = storedvolume_.hrg.stop.inl = lineidx;
	    StepInterval<int> trcrg; StepInterval<float> zrg;
	    if ( !lset->getRanges( lineidx, trcrg, zrg ) )
		mErrRet("Cannot get needed trace range from 2D line set")
	    else
	    {
		storedvolume_.hrg.start.crl = trcrg.start;
		storedvolume_.hrg.stop.crl = trcrg.stop;
		storedvolume_.zrg = zrg;
	    }
	}
    }

    status_ = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace( BinID startpos, bool firstcheck )
{
    if ( alreadymoved_ )
	return 1;

    if ( status_==Nada && isondisc_ )
	return -1;

    if ( status_==StorageOpened )
    {
	if ( !setMSCProvSelData() )
	    return -1;

	status_ = Ready;
    }

    if ( !useshortcuts_ )
    {
	if ( getDesc().is2D() )
	    prevtrcnr_ = currentbid_.crl;

	bool validstartpos = startpos != BinID(-1,-1);
	if ( validstartpos && curtrcinfo_ && curtrcinfo_->binid == startpos )
	{
	    alreadymoved_ = true;
	    return 1;
	}
    }

    bool advancefurther = true;
    while ( advancefurther )
    {
	if ( isondisc_ )
	{
	    SeisMSCProvider::AdvanceState res = mscprov_->advance();
	    switch ( res )
	    {
		case SeisMSCProvider::Error:	{ errmsg_ = mscprov_->errMsg();
						      return -1; }
		case SeisMSCProvider::EndReached:	return 0;
		case SeisMSCProvider::Buffering:	continue;
						//TODO return 'no new position'

		case SeisMSCProvider::NewPosition:
		{
		    if ( useshortcuts_ )
			{ advancefurther = false; continue; }

		    SeisTrc* trc = mscprov_->get( 0, 0 );
		    if ( !trc ) continue; // should not happen

		    registerNewPosInfo( trc, startpos, firstcheck,
			    		advancefurther );
		}
	    }
	}
	else
	{
	    SeisTrc* trc = getTrcFromPack( BinID(0,0), 1 );
	    if ( !trc ) return 0;

	    status_ = Ready;
	    registerNewPosInfo( trc, startpos, firstcheck, advancefurther );
	}
    }

    setCurrentPosition( currentbid_ );
    alreadymoved_ = true;
    return 1;
}


void StorageProvider::registerNewPosInfo( SeisTrc* trc, const BinID& startpos,
					  bool firstcheck, bool& advancefurther)
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

    curtrcinfo_ = 0;
    const SeisTrcInfo& newti = trc->info();
    currentbid_ = desc_.is2D()? BinID( 0, newti.nr ) : newti.binid;
    trcinfobid_ = newti.binid;
    if ( firstcheck || startpos == BinID(-1,-1) || currentbid_ == startpos
	    || newti.binid == startpos )
    {
	advancefurther = false;
	curtrcinfo_ = &trc->info();
    }
}


#define mAdjustToAvailStep( dir )\
{\
    if ( res.hrg.step.dir>1 )\
    {\
	float remain = ( possiblevolume_->hrg.start.dir - res.hrg.start.dir ) %\
	    		res.hrg.step.dir;\
	if ( !mIsZero( remain, 1e-3 ) )\
	    res.hrg.start.dir = possiblevolume_->hrg.start.dir + \
				mNINT32(remain +0.5) *res.hrg.step.dir;\
    }\
}

bool StorageProvider::getPossibleVolume( int, CubeSampling& globpv )
{
    if ( !possiblevolume_ ) 
	possiblevolume_ = new CubeSampling;

    *possiblevolume_ = storedvolume_;
    globpv.limitToWithUdf( *possiblevolume_ );

    if ( mscprov_ && mscprov_->is2D() )
    {
	globpv.hrg.stop.inl = globpv.hrg.start.inl = 0;
	globpv.hrg.setCrlRange( storedvolume_.hrg.crlRange() );
	return globpv.nrCrl() > 0;
    }
    else
	globpv.limitToWithUdf( *possiblevolume_ );

    const bool haveinls = globpv.hrg.inlRange().width(false) >= 0;
    const bool havecrls = globpv.hrg.crlRange().width(false) >= 0;
    return haveinls && havecrls;
}


bool StorageProvider::initMSCProvider()
{
    if ( !mscprov_ || !mscprov_->prepareWork() )
	return false;

    updateStorageReqs();
    return true;
}


#define setBufStepout( prefix ) \
{ \
    if ( ns.inl <= prefix##bufferstepout_.inl \
	    && ns.crl <= prefix##bufferstepout_.crl ) \
	return; \
\
    if ( ns.inl > prefix##bufferstepout_.inl ) \
	prefix##bufferstepout_.inl = ns.inl; \
    if ( ns.crl > prefix##bufferstepout_.crl ) \
	prefix##bufferstepout_.crl = ns.crl;\
}


void StorageProvider::setReqBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(req);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::setDesBufStepout( const BinID& ns, bool wait )
{
    setBufStepout(des);
    if ( !wait )
	updateStorageReqs();
}


void StorageProvider::updateStorageReqs( bool )
{
    if ( !mscprov_ ) return;

    mscprov_->setStepout( desbufferstepout_.inl, desbufferstepout_.crl, false );
    mscprov_->setStepout( reqbufferstepout_.inl, reqbufferstepout_.crl, true );
}


SeisMSCProvider* StorageProvider::getMSCProvider( bool& needmscprov) const
{
    needmscprov = isondisc_;
    return mscprov_;
}


bool StorageProvider::setMSCProvSelData()
{
    if ( !mscprov_ ) return false;

    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.psIOProv() ) return false;

    const bool is2d = reader.is2D();
    const bool haveseldata = seldata_ && !seldata_->isAll();

    if ( haveseldata && seldata_->type() == Seis::Table )
	return setTableSelData();

    if ( is2d )
	return set2DRangeSelData(); 

    if ( !desiredvolume_ )
    {
	for ( int idp=0; idp<parents_.size(); idp++ )
	{
	    if ( !parents_[idp] ) continue;

	    if ( parents_[idp]->getDesiredVolume() )
	    {
		setDesiredVolume( *parents_[idp]->getDesiredVolume() );
		break;
	    }
	}
	if ( !desiredvolume_ )
	    return true;
    }

    if ( !checkDesiredVolumeOK() )
	return false;

    CubeSampling cs;
    cs.hrg.start.inl = 
	    desiredvolume_->hrg.start.inl < storedvolume_.hrg.start.inl ?
	    storedvolume_.hrg.start.inl : desiredvolume_->hrg.start.inl;
    cs.hrg.stop.inl = 
	    desiredvolume_->hrg.stop.inl > storedvolume_.hrg.stop.inl ?
	    storedvolume_.hrg.stop.inl : desiredvolume_->hrg.stop.inl;
    cs.hrg.stop.crl = 
	    desiredvolume_->hrg.stop.crl > storedvolume_.hrg.stop.crl ?
	    storedvolume_.hrg.stop.crl : desiredvolume_->hrg.stop.crl;
    cs.hrg.start.crl = 
	    desiredvolume_->hrg.start.crl < storedvolume_.hrg.start.crl ?
	    storedvolume_.hrg.start.crl : desiredvolume_->hrg.start.crl;
    cs.zrg.start = desiredvolume_->zrg.start < storedvolume_.zrg.start ?
		    storedvolume_.zrg.start : desiredvolume_->zrg.start;
    cs.zrg.stop = desiredvolume_->zrg.stop > storedvolume_.zrg.stop ?
		     storedvolume_.zrg.stop : desiredvolume_->zrg.stop;

    reader.setSelData( haveseldata ? seldata_->clone()
	   			   : new Seis::RangeSelData(cs) );

    SeisTrcTranslator* transl = reader.seisTranslator();
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] ) 
	    transl->componentInfo()[idx]->destidx = -1;
    }

    return true;
}


bool StorageProvider::setTableSelData()
{
    if ( !isondisc_ ) return false;	//in this case we might not use a table

    Seis::SelData* seldata = seldata_->clone();
    seldata->extendZ( extraz_ );
    SeisTrcReader& reader = mscprov_->reader();
    if ( reader.is2D() )
    {
	const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
	seldata->lineKey().setAttrName( lk.attrName() );
    }
    reader.setSelData( seldata );
    SeisTrcTranslator* transl = reader.seisTranslator();
    if ( !transl ) return false;
    for ( int idx=0; idx<outputinterest_.size(); idx++ )
    {
	if ( !outputinterest_[idx] && transl->componentInfo().size()>idx ) 
	    transl->componentInfo()[idx]->destidx = -1;
    }
    return true;
}


bool StorageProvider::set2DRangeSelData()
{
    if ( !isondisc_ ) return false;

    mDynamicCastGet(const Seis::RangeSelData*,rsd,seldata_)
    Seis::RangeSelData* seldata = rsd ? (Seis::RangeSelData*)rsd->clone()
				      : new Seis::RangeSelData( true );
    SeisTrcReader& reader = mscprov_->reader();
    Seis2DLineSet* lset = reader.lineSet();
    if ( !lset )
    {
	if ( !rsd && seldata ) delete seldata;
	return false;
    }

    seldata->lineKey().setAttrName( curlinekey_.attrName() );
    if ( !curlinekey_.lineName().isEmpty() )
    {
	seldata->lineKey().setLineName( curlinekey_.lineName() );
	int idx = lset->indexOf( curlinekey_ );
	StepInterval<float> lsetzrg; StepInterval<int> trcrg;
	if ( idx >= 0 && lset->getRanges(idx,trcrg,lsetzrg) )
	{
	    if ( !checkDesiredTrcRgOK(trcrg,lsetzrg) )
		return false;
	    Interval<int> rg( 0, 0 );
	    seldata->setInlRange( rg );
	    rg.start = desiredvolume_->hrg.start.crl < trcrg.start?
			trcrg.start : desiredvolume_->hrg.start.crl;
	    rg.stop = desiredvolume_->hrg.stop.crl > trcrg.stop ?
			trcrg.stop : desiredvolume_->hrg.stop.crl;
	    seldata->setCrlRange( rg );
	    Interval<float> zrg;
	    zrg.start = desiredvolume_->zrg.start < lsetzrg.start ?
			lsetzrg.start : desiredvolume_->zrg.start;
	    zrg.stop = desiredvolume_->zrg.stop > lsetzrg.stop ?
			lsetzrg.stop : desiredvolume_->zrg.stop;
	    seldata->setZRange( zrg );
	}
	reader.setSelData( seldata );
    }
    else if ( !rsd && seldata )
	delete seldata;

    return true;
}


#define mInitErrMsg() \
    errmsg_ = "'"; errmsg_ += desc_.userRef(); errmsg_ += "'"; \
    errmsg_ += " contains no data in selected area:\n"

#define mAdd2ErrMsg(varwrong,s,start,stop) \
    if ( varwrong ) \
    { \
	errmsg_ += s; errmsg_ += " range is: "; \
	errmsg_ += start; errmsg_ += "-"; errmsg_ += stop; \
	errmsg_ += "\n"; \
    }

bool StorageProvider::checkDesiredVolumeOK()
{
    if ( !desiredvolume_ )
	return true;

    const bool inlwrong =
	desiredvolume_->hrg.start.inl > storedvolume_.hrg.stop.inl
     || desiredvolume_->hrg.stop.inl < storedvolume_.hrg.start.inl;
    const bool crlwrong =
	desiredvolume_->hrg.start.crl > storedvolume_.hrg.stop.crl
     || desiredvolume_->hrg.stop.crl < storedvolume_.hrg.start.crl;
    const bool zwrong =
	desiredvolume_->zrg.start > storedvolume_.zrg.stop
     || desiredvolume_->zrg.stop < storedvolume_.zrg.start;

    if ( !inlwrong && !crlwrong && !zwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(inlwrong,"Inline",
	    	storedvolume_.hrg.start.inl,storedvolume_.hrg.stop.inl)
    mAdd2ErrMsg(crlwrong,"Crossline",
	    	storedvolume_.hrg.start.crl,storedvolume_.hrg.stop.crl)
    mAdd2ErrMsg(zwrong,"Z",storedvolume_.zrg.start,storedvolume_.zrg.stop)
    return false;
}


bool StorageProvider::checkDesiredTrcRgOK( StepInterval<int> trcrg, 
				   StepInterval<float>zrg )
{
    if ( !desiredvolume_ )
    {
	errmsg_ = "internal error, '"; errmsg_ += desc_.userRef(); errmsg_ += "'";
	errmsg_ += " has no desired volume\n";
	return false;
    }
    
    const bool trcrgwrong =
	desiredvolume_->hrg.start.crl > trcrg.stop
     || desiredvolume_->hrg.stop.crl < trcrg.start;
    const bool zwrong =
	desiredvolume_->zrg.start > zrg.stop
     || desiredvolume_->zrg.stop < zrg.start;

    if ( !trcrgwrong && !zwrong )
	return true;

    mInitErrMsg();
    mAdd2ErrMsg(trcrgwrong,"Trace",trcrg.start,trcrg.stop)
    mAdd2ErrMsg(zwrong,"Z",zrg.start,zrg.stop)
    return false;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int z0, int nrsamples, int threadid ) const
{
    const BinID bidstep = getStepoutStep();
    const SeisTrc* trc = 0;
    
    if ( isondisc_)
	trc = mscprov_->get( relpos.inl/bidstep.inl, relpos.crl/bidstep.crl );
    else
	trc = getTrcFromPack( relpos, 0 );

    if ( !trc || !trc->size() )
	return false;

    if ( desc_.is2D() && seldata_ && seldata_->type() == Seis::Table )
    {
	Interval<float> deszrg = desiredvolume_->zrg;
	Interval<float> poszrg = possiblevolume_->zrg;
	const float desonlyzrgstart = deszrg.start - poszrg.start;
	const float desonlyzrgstop = deszrg.stop - poszrg.stop;
	Interval<float> trcrange = trc->info().sampling.interval(trc->size());
	const float diffstart = z0*refstep_ - trcrange.start;
	const float diffstop = (z0+nrsamples-1)*refstep_ - trcrange.stop;
	bool isdiffacceptable =
	    ( (mIsEqual(diffstart,0,refstep_/100) || diffstart>0)
	      || diffstart >= desonlyzrgstart )
	 && ( (mIsEqual(diffstop,0,refstep_/100) || diffstop<0 )
	      || diffstop<=desonlyzrgstop );
	if ( !isdiffacceptable )
	    return false;
    }

    return fillDataHolderWithTrc( trc, output );
}


SeisTrc* StorageProvider::getTrcFromPack( const BinID& relpos, int relidx) const
{
    const LineKey lk( desc_.getValParam(keyStr())->getStringValue(0) );
    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( !linenm || *linenm != '#' )
	return 0;

    DataPack::FullID fid( linenm+1 );
    DataPack* dtp = DPM( fid ).obtain( DataPack::getID(fid), false );
    mDynamicCastGet(SeisTrcBufDataPack*,stbdtp, dtp)
    if ( !stbdtp )
	return 0;

    int trcidx = stbdtp->trcBuf().find(currentbid_+relpos, desc_.is2D());
    if ( trcidx+relidx >= stbdtp->trcBuf().size() || trcidx+relidx<0 )
	return 0;

    return stbdtp->trcBuf().get( trcidx + relidx );
}


bool StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc, 
					     const DataHolder& data ) const
{
    const int z0 = data.z0_;
    float exacttime = 0;
    float extrazfromsamppos = 0;
    BoolTypeSet isclass( outputinterest_.size(), true );
    if ( needinterp_ )
    {
	checkClassType( trc, isclass );
	extrazfromsamppos = getExtraZFromSampInterval( z0, data.nrsamples_ );
	const_cast<DataHolder&>(data).extrazfromsamppos_ = extrazfromsamppos;
    }
    
    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001 * trc->info().sampling.step );
    for ( int idx=0; idx<data.nrsamples_; idx++ )
    {
	const float curt = (float)(z0+idx)*refstep_ + extrazfromsamppos;
	int compidx = -1;
	for ( int idy=0; idy<outputinterest_.size(); idy++ )
	{
	    if ( outputinterest_[idy] )
	    {
		compidx++;
		const int compnr = desc_.is2D() ? idy : compidx;
		const float val = trcrange.includes(curt,false) ? 
		   ( isclass[idy] ? trc->get(trc->nearestSample(curt), compnr)
				  : trc->getValue(curt, compnr) )
		   : mUdf(float);

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
    if ( stepoutstep_.inl >= 0 )
	return stepoutstep_;

    BinID& sos = const_cast<StorageProvider*>(this)->stepoutstep_;
    if ( mscprov_ )
    {
	PtrMan<Seis::Bounds> bds = mscprov_->reader().getBounds();
	if ( bds )
	{
	    sos.inl = bds->step( true );
	    sos.crl = bds->step( false );
	}
    }
    else
	sos.inl = sos.crl = 1;

    return stepoutstep_;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    if ( !isondisc_ || !mscprov_ ) return;

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return;

    const Seis2DLineSet* lset = reader.lineSet();
    const int idx = lset->indexOf( curlinekey_ );
    StepInterval<int> trcrg;
    StepInterval<float> zrg;
    if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
    {
	storedvolume_.hrg.start.crl = trcrg.start;
	storedvolume_.hrg.stop.crl = trcrg.stop;
	storedvolume_.zrg.start = zrg.start;
	storedvolume_.zrg.stop = zrg.stop;
	storedvolume_.zrg.step = zrg.step;
    }
}


void StorageProvider::fillDataCubesWithTrc( DataCubes* dc ) const
{
    if ( !mscprov_ ) return;
    const SeisTrc* trc = mscprov_->get(0,0);
    if ( !trc ) return;

    Interval<float> trcrange = trc->info().sampling.interval(trc->size());
    trcrange.widen( 0.001 * trc->info().sampling.step );
    const BinID bid = trc->info().binid;
    if ( !dc->includes(bid) )
	return;

    const int inlidx = dc->inlsampling_.nearestIndex( bid.inl );
    const int crlidx = dc->crlsampling_.nearestIndex( bid.crl );
    for ( int idz=0; idz<dc->getZSz(); idz++ )
    {
	const float curt = (dc->z0_+idz) * dc->zstep_;
	int cubeidx = -1;
	for ( int idx=0; idx<outputinterest_.size(); idx++ )
	{
	    if ( !outputinterest_[idx] )
		continue;

	    cubeidx++;
	    if ( cubeidx >= dc->nrCubes() && !dc->addCube(mUdf(float)) )
		continue;

	    if ( !trcrange.includes(curt,false) )
		continue;

	    //the component index inthe trace is depending on outputinterest_,
	    //thus is the same as cubeidx
	    const float val = trc->getValue( curt, cubeidx );
	    dc->setValue( cubeidx, inlidx, crlidx, idz, val );
	}
    }
} 


void StorageProvider::checkClassType( const SeisTrc* trc,
				      BoolTypeSet& isclass ) const
{
    int idx = 0;
    bool foundneed = true;
    while ( idx<trc->size() && foundneed )
    {
	foundneed = false;
	int compidx = -1;
	for ( int ido=0; ido<outputinterest_.size(); ido++ )
	{
	    if ( outputinterest_[ido] )
	    {
		compidx++;
		if ( isclass[ido] )
		{
		    foundneed = true;
		    const float val  = trc->get( idx, compidx );
		    if ( !holdsClassValue( val) )
			isclass[ido] = false;
		}
	    }
	}
	idx++;
    }
}


float StorageProvider::getMaxDistBetwTrcs() const
{
    if ( !mscprov_ ) return mUdf(float);

    const SeisTrcReader& reader = mscprov_->reader();
    if ( !reader.is2D() ) return mUdf(float);

    const Seis2DLineSet* lset = reader.lineSet();
    if ( !lset )
	return mUdf(float);

    S2DPOS().setCurLineSet( lset->name() );
    PosInfo::LineSet2DData ls2ddata;
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = ls2ddata.addLine(lset->lineName(idx));
	S2DPOS().getGeometry( linegeom );
	if ( linegeom.positions().isEmpty() )
	{
	    ls2ddata.removeLine( lset->lineName(idx) );
	    return mUdf(float);
	}
    }

    double maxdistsq = 0;
    for ( int lidx=0; lidx<ls2ddata.nrLines(); lidx++ )
    {
	const TypeSet<PosInfo::Line2DPos>& posns
	    			= ls2ddata.lineData(lidx).positions();
	for ( int pidx=1; pidx<posns.size(); pidx++ )
	{
	    const double distsq =
		posns[pidx].coord_.sqDistTo( posns[pidx-1].coord_ );
	    if ( distsq > maxdistsq )
		maxdistsq = distsq;
	}
    }

    return maxdistsq < 1e-3 ? mUdf(float) : (float)sqrt(maxdistsq);
}


void StorageProvider::getCompNamesFakeToKeepHeadersOK( 
					BufferStringSet& nms ) const
{
    updateDescAndGetCompNms( desc_, &nms );
}


}; // namespace Attrib
