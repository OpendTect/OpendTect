/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/


#include "seisparallelreader.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "cbvsreadmgr.h"
#include "convmemvalseries.h"
#include "datapackbase.h"
#include "nrbytes2string.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "odsysmem.h"
#include "posinfo.h"
#include "samplingdata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "threadwork.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include <string.h>

namespace Seis
{


static bool addComponents( RegularSeisDataPack& dp, const IOObj& ioobj,
			   TypeSet<int>& selcomponents, od_ostream* logstrm )
{
    BufferStringSet cnames;
    SeisIOObjInfo::getCompNames( ioobj.key(), cnames );
    const int nrcomp = selcomponents.size();
    od_int64 totmem, freemem;
    OD::getSystemMemory( totmem, freemem );
    NrBytesToStringCreator nbstr( totmem );
    const od_uint64 reqsz = nrcomp * dp.sampling().totalNr() *
			    dp.getDataDesc().nrBytes();
			    // dp.nrKBytes() cannot be used before allocation

    od_ostream& ostrm = logstrm ? *logstrm : od_ostream::logStream();
    BufferString memszstr( nbstr.getString( reqsz ) );
    if ( reqsz >= freemem )
    {
	ostrm << od_newline << "Insufficient memory for allocating ";
	ostrm << memszstr << od_endl;
	return false;
    }

    if ( logstrm )
	ostrm << od_newline << "Allocating " << memszstr << od_newline;

    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const int cidx = selcomponents[idx];
	const char* cnm = cnames.size()>1 && cnames.validIdx(cidx) ?
			  cnames.get(cidx).buf() : BufferString::empty().buf();

	// assembles composite "<attribute>|<component>" name
	const StringPair compstr( ioobj.name().str(), cnm );
	if ( !dp.addComponent( compstr.getCompString() ) )
	    return false;
    }

    return true;
}


ParallelReader::ParallelReader( const IOObj& ioobj, const TrcKeyZSampling& cs )
    : dp_(0)
    , bidvals_(0)
    , outcomponents_(0)
    , tkzs_(cs)
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hsamp_.totalNr() )
{
    SeisIOObjInfo seisinfo( ioobj );
    const int nrcomponents = seisinfo.nrComponents();
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;
}


ParallelReader::ParallelReader( const IOObj& ioobj,
				BinIDValueSet& bidvals,
				const TypeSet<int>& components )
    : dp_(0)
    , components_( components )
    , outcomponents_(0)
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{
    errmsg_ = uiStrings::phrReading( ioobj_->uiName() );
}


ParallelReader::~ParallelReader()
{
    delete ioobj_;
    delete outcomponents_;
}


bool ParallelReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete outcomponents_;
    outcomponents_ = new TypeSet<int>( compnrs );

    return true;
}


void ParallelReader::setDataPack( RegularSeisDataPack* dp )
{
    dp_ = dp;
    DPM( DataPackMgr::SeisID() ).add( dp );
}


RegularSeisDataPack* ParallelReader::getDataPack()
{
    return dp_;
}


uiString ParallelReader::uiNrDoneText() const
{ return tr("Traces read"); }


uiString ParallelReader::uiMessage() const
{
    return errmsg_.isEmpty() ? tr("Reading volume \'%1\'").arg(ioobj_->uiName())
			     : errmsg_;
}


bool ParallelReader::doPrepare( int nrthreads )
{
    uiString allocprob = tr("Cannot allocate memory");

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not tested. Run through step by step, make "
		"sure everything is OK and remove this warning.");
	const int nrvals = 1+components_.size();
        if ( bidvals_->nrVals()!=nrvals )
	{
	    if ( !bidvals_->setNrVals( nrvals ) )
	    {
		errmsg_ = allocprob;
		return false;
	    }
	}
    }
    else if ( !dp_ )
    {
	dp_ = new RegularSeisDataPack(0);
	dp_->setSampling( tkzs_ );
	DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );

	if ( !addComponents(*dp_,*ioobj_,components_,0) )
	{
	    errmsg_ = allocprob;
	    return false;
	}
    }

    return true;
}


bool ParallelReader::doWork( od_int64 start, od_int64 stop, int threadid )
{
    PtrMan<IOObj> ioobj = ioobj_->clone();
    PtrMan<SeisTrcReader> reader = new SeisTrcReader( ioobj );
    if ( !reader )
    {
	errmsg_ = tr("Cannot open storage");
	return false;
    }

    if ( !reader->prepareWork() )
    {
	errmsg_ = reader->errMsg();
	return false;
    }

    if ( !threadid && !dp_->is2D() )
    {
	PosInfo::CubeData cubedata;
	if ( reader->get3DGeometryInfo(cubedata) )
	{
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );
	}
    }

    //Set ranges, at least z-range
    mDynamicCastGet( SeisTrcTranslator*, translator, reader->translator() );
    if ( !translator || !translator->supportsGoTo() )
    {
	errmsg_ = tr("Storage does not support random access");
	return false;
    }

    const TypeSet<int>& outcompnrs = outcomponents_
				   ? *outcomponents_
				   : components_;
    TrcKeySamplingIterator iter;
    BinIDValueSet::SPos bidvalpos;
    BinID curbid;
    if ( bidvals_ )
    {
        bidvalpos = bidvals_->getPos( start );
        if ( !bidvalpos.isValid() )
            return false;
    }
    else
    {
	iter.setSampling( tkzs_.hsamp_ );
	iter.setCurrentPos( start );
    }

    SeisTrc trc;

#define mUpdateInterval 100
    int nrdone = 0;
    for ( od_int64 idx=start; idx<=stop; idx++, nrdone++ )
    {
	if ( nrdone>mUpdateInterval )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;

	    if ( !shouldContinue() )
		return false;
	}

	if ( ( bidvals_ && !bidvals_->next(bidvalpos,false) ) ||
	     ( !bidvals_ && !iter.next() ) )
	    return false;

	curbid = bidvals_ ? bidvals_->getBinID( bidvalpos )
			  : iter.curBinID();

        if ( translator->goTo( curbid )
	  && reader->get( trc )
	  && trc.info().binID() == curbid )
        {
	    const StepInterval<float> trczrg = trc.zRange();

            if ( bidvals_ )
            {
		float* vals = bidvals_->getVals(bidvalpos);
		const float z = vals[0];

		if ( !mIsUdf(z) && trczrg.includes( z, false ) )
		{
		    for ( int idcx=outcompnrs.size()-1; idcx>=0; idcx-- )
		    {
			const int idc = outcompnrs[idcx];
			vals[idc+1] = trc.getValue( z, components_[idcx] );
		    }
		}
            }
            else
            {
		const int inlidx = tkzs_.hsamp_.inlIdx( curbid.inl() );
		const int crlidx = tkzs_.hsamp_.crlIdx( curbid.crl() );

		for ( int idz=dp_->sampling().nrZ()-1; idz>=0; idz--)
		{
		    float val;
		    const double z = tkzs_.zsamp_.atIndex( idz );
		    if ( trczrg.includes( z, false ) )
		    {
			for ( int idcx=outcompnrs.size()-1; idcx>=0; idcx-- )
			{
			    val = trc.getValue( (float) z, components_[idcx] );
			    if ( !mIsUdf(val) )
			    {
				const int idc = outcompnrs[idcx];
				if ( idc < dp_->nrComponents() )
				{
				    Array3D<float>& arr3d = dp_->data( idc );
				    arr3d.set( inlidx, crlidx, idz, val);
				}
			    }
			}
		    }
		}
            }
        }
    }

    addToNrDone( nrdone );

    return true;
}


bool ParallelReader::doFinish( bool success )
{ return success; }



// ParallelReader2D (probably replace by a Sequential reader)
ParallelReader2D::ParallelReader2D( const IOObj& ioobj, Pos::GeomID geomid,
				    const TrcKeyZSampling* tkzs,
				    const TypeSet<int>* comps )
    : geomid_(geomid)
    , ioobj_(ioobj.clone())
    , dc_(DataCharacteristics::Auto)
    , dpclaimed_(false)
    , scaler_(0)
    , dp_(0)
{
    if ( comps )
	components_ = *comps;

    if ( tkzs )
	tkzs_ = *tkzs;

    totalnr_ = tkzs_.isEmpty() ? 1 : tkzs_.hsamp_.totalNr();
}


bool ParallelReader2D::doPrepare( int )
{
    return init();
}


bool ParallelReader2D::init()
{
    const SeisIOObjInfo info( *ioobj_ );
    if ( !info.isOK() ) return false;

    info.getDataChar( dc_ );
    if ( components_.isEmpty() )
    {
	const int nrcomps = info.nrComponents( geomid_ );
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }

    if ( tkzs_.isEmpty() )
    {
	StepInterval<int> trcrg;
	info.getRanges( geomid_, trcrg, tkzs_.zsamp_ );
	tkzs_.hsamp_.set( Interval<int>(geomid_,geomid_), trcrg );
    }

    totalnr_ = tkzs_.hsamp_.totalNr();

    dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,true), &dc_ );
    dp_->setSampling( tkzs_ );
    if ( scaler_ )
	dp_->setScaler( *scaler_ );

    if ( !addComponents(*dp_,*ioobj_,components_,0) )
    {
	dp_->unRef(); dp_ = 0;
	return false;
    }

    msg_ = uiStrings::phrReading( ioobj_->uiName() );

    return true;
}


ParallelReader2D::~ParallelReader2D()
{
    delete ioobj_;
    delete scaler_;
}


void ParallelReader2D::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void ParallelReader2D::setScaler( Scaler* scaler )
{
    delete scaler_;
    scaler_ = scaler;
}


uiString ParallelReader2D::uiNrDoneText() const
{ return tr("Traces read"); }

uiString ParallelReader2D::uiMessage() const
{ return msg_.isEmpty() ? tr("Reading") : msg_; }

od_int64 ParallelReader2D::nrIterations() const
{ return totalnr_; }


bool ParallelReader2D::doWork( od_int64 start, od_int64 stop, int threadid )
{
    if ( !dp_ || dp_->nrComponents()==0 )
	return false;

    PtrMan<IOObj> ioobj = ioobj_->clone();
    const Seis2DDataSet dataset( *ioobj );
    const int lidx = dataset.indexOf( geomid_ );
    if ( lidx<0 ) return false;

    const char* fnm = SeisCBVS2DLineIOProvider::getFileName( *ioobj,
							dataset.geomID(lidx) );
    PtrMan<CBVSSeisTrcTranslator> trl =
	CBVSSeisTrcTranslator::make( fnm, false, true );
    if ( !trl ) return false;

    SeisTrc trc;
    BinID curbid;
    StepInterval<int> trcrg = tkzs_.hsamp_.crlRange();
    trl->toStart();
    curbid = trl->readMgr()->binID();

    const Scaler* scaler = dp_->getScaler();
    for ( int idc=0; idc<dp_->nrComponents(); idc++ )
    {
	Array3D<float>& arr = dp_->data( idc );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();

	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    curbid.crl() = trcrg.atIndex( mCast(int,idx) );
	    if ( trl->goTo(curbid) && trl->read(trc) )
	    {
		if ( scaler )
		{
		    SeisTrcPropChg seistrcpropchg( trc, idc );
		    seistrcpropchg.scale( *scaler );
		}

		const BinDataDesc trcdatadesc =
			trc.data().getInterpreter(idc)->dataChar();
		if ( storarr && dp_->getDataDesc()==trcdatadesc )
		{
		    const DataBuffer* databuf = trc.data().getComponent( idc );
		    const int bytespersamp = databuf->bytesPerElement();
		    const od_int64 offset =
			arr.info().getOffset( 0, mCast(int,idx), 0 );
		    char* dststartptr = storarr + offset*bytespersamp;

		    for ( int zidx=0; zidx<tkzs_.zsamp_.nrSteps()+1; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			const int trczidx = trc.nearestSample( zval );
			const unsigned char* srcptr =
				databuf->data() + trczidx*bytespersamp;
			char* dstptr = dststartptr + zidx*bytespersamp;
			// Checks if amplitude equals undef value of underlying
			// data type as the array is initialized with undefs.
			if ( memcmp(dstptr,srcptr,bytespersamp) )
			    OD::sysMemCopy(dstptr,srcptr,bytespersamp );
			else
			    arr.set( 0, (int)idx, zidx, trc.getValue(zval,idc));
		    }
		}
		else
		{
		    for ( int zidx=0; zidx<tkzs_.zsamp_.nrSteps()+1; zidx++ )
		    {
			const float zval = tkzs_.zsamp_.atIndex( zidx );
			arr.set( 0, (int)idx, zidx, trc.getValue(zval,idc) );
		    }
		}
	    }

	    addToNrDone( 1 );
	}
    }

    return true;
}


bool ParallelReader2D::doFinish( bool success )
{ return success; }


RegularSeisDataPack* ParallelReader2D::getDataPack()
{
    dpclaimed_ = true;
    return dp_;
}



// SequentialReader
class ArrayFiller : public Task
{
public:
ArrayFiller( SeisTrc& trc, const TypeSet<int>& components,
	     const TypeSet<int>& outcomponents, RegularSeisDataPack& dp,
	     bool is2d )
    : trc_(trc)
    , components_(components)
    , outcomponents_(outcomponents)
    , dp_(dp),is2d_(is2d)
{}


~ArrayFiller()
{ delete &trc_; }

bool execute()
{
    const int idx0 = is2d_ ? 0
		: dp_.sampling().hsamp_.lineIdx( trc_.info().inl() );
    const int idx1 = dp_.sampling().hsamp_.trcIdx( is2d_ ? trc_.info().nr_
							 : trc_.info().crl() );

    const StepInterval<float>& zsamp = dp_.sampling().zsamp_;

    for ( int cidx=0; cidx<outcomponents_.size(); cidx++ )
    {
	const int idcin = components_[cidx];
	const int idcout = outcomponents_[cidx];
	Array3D<float>& arr = dp_.data( idcout );
	ValueSeries<float>* stor = arr.getStorage();
	mDynamicCastGet(ConvMemValueSeries<float>*,storptr,stor);
	char* storarr = storptr ? storptr->storArr() : (char*)stor->arr();
	if ( dp_.getScaler() )
	{
	    SeisTrcPropChg seistrcpropchg( trc_, idcin );
	    seistrcpropchg.scale( *dp_.getScaler() );
	}

	const BinDataDesc trcdatadesc =
		trc_.data().getInterpreter(idcin)->dataChar();
	if ( storarr && dp_.getDataDesc()==trcdatadesc )
	{
	    const DataBuffer* databuf = trc_.data().getComponent( idcin );
	    const int bytespersamp = databuf->bytesPerElement();
	    const od_int64 offset = arr.info().getOffset( idx0, idx1, 0 );
	    char* dststartptr = storarr + offset*bytespersamp;

	    for ( int zidx=0; zidx<zsamp.nrSteps()+1; zidx++ )
	    {
		// Check if amplitude equals undef value of underlying data
		// type knowing that array has been initialized with undefs
		const float zval = zsamp.atIndex( zidx );
		const int trczidx = trc_.nearestSample( zval );
		const unsigned char* srcptr =
			databuf->data() + trczidx*bytespersamp;
		char* dstptr = dststartptr + zidx*bytespersamp;
		if ( memcmp(dstptr,srcptr,bytespersamp) )
		    OD::sysMemCopy(dstptr,srcptr,bytespersamp );
		else
		    arr.set( idx0, idx1, zidx, trc_.getValue(zval,idcin) );
	    }
	}
	else
	{
	    for ( int zidx=0; zidx<zsamp.nrSteps()+1; zidx++ )
	    {
		const float zval = zsamp.atIndex( zidx );
		arr.set( idx0, idx1, zidx, trc_.getValue(zval,idcin) );
	    }
	}
    }

    return true;
}

protected:

    SeisTrc&			trc_;
    const TypeSet<int>&		components_;
    const TypeSet<int>&		outcomponents_;
    RegularSeisDataPack&	dp_;
    bool			is2d_;
};


SequentialReader::SequentialReader( const IOObj& ioobj,
				    const TrcKeyZSampling* tkzs,
				    const TypeSet<int>* comps )
    : Executor("Volume Reader")
    , ioobj_(ioobj.clone())
    , dp_(0)
    , outcomponents_(0)
    , sd_(0)
    , scaler_(0)
    , rdr_(*new SeisTrcReader(ioobj_))
    , dc_(DataCharacteristics::Auto)
    , initialized_(false)
    , is2d_(false)
{
    if ( comps )
	components_ = *comps;

    if ( tkzs )
	tkzs_ = *tkzs;

    queueid_ = Threads::WorkManager::twm().addQueue(
				Threads::WorkManager::MultiThread,
				"SequentialReader" );
}


SequentialReader::~SequentialReader()
{
    delete &rdr_; delete ioobj_;
    delete outcomponents_;
    delete scaler_;

    Threads::WorkManager::twm().removeQueue( queueid_, false );
}


uiString SequentialReader::uiNrDoneText() const
{
    return uiStrings::phrJoinStrings( uiStrings::sTrace(mPlural), tr("read") );
}


bool SequentialReader::setOutputComponents( const TypeSet<int>& compnrs )
{
    if ( compnrs.size() != components_.size() )
	return false;

    delete outcomponents_;
    outcomponents_ = new TypeSet<int>( compnrs );

    return true;
}


void SequentialReader::setDataChar( DataCharacteristics::UserType type )
{ dc_ = DataCharacteristics(type); }


void SequentialReader::setScaler( Scaler* newsc )
{
    delete scaler_;
    scaler_ = newsc;
//    rdr_.forceFloatData( scaler_ ); // Not sure if needed
}


RegularSeisDataPack* SequentialReader::getDataPack()
{ return dp_; }


#define mSetSelData() \
{ \
    sd_ = new Seis::RangeSelData( tkzs_ ); \
    rdr_.setSelData( sd_ ); \
    if ( !rdr_.prepareWork() ) \
    { \
	msg_ = rdr_.errMsg(); \
	return false; \
    } \
}

bool SequentialReader::init()
{
    if ( initialized_ )
	return true;

    msg_ = tr("Initializing reader");
    SeisIOObjInfo info( ioobj_ );
    if ( !info.isOK() ) return false;
    is2d_ = info.is2D();
    info.getDataChar( dc_ );
    if ( components_.isEmpty() )
    {
	const int nrcomps = info.nrComponents();
	for ( int idx=0; idx<nrcomps; idx++ )
	    components_ += idx;
    }

    if ( is2d_ && !TrcKey::is2D(tkzs_.hsamp_.survid_) )
    {
	pErrMsg("TrcKeySampling for 2D data needed with GeomID as lineNr");
	return false;
    }

    if ( !dp_ )
    {
	if ( !is2d_ )
	{
	    TrcKeyZSampling storedtkzs;
	    info.getRanges( storedtkzs );
	    if ( tkzs_.isDefined() )
		tkzs_. limitTo( storedtkzs );
	    else
		tkzs_ = storedtkzs;
	}
	else
	{
	    Pos::GeomID geomid = tkzs_.hsamp_.start_.lineNr();
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    if ( !info.getRanges(geomid,trcrg,zrg) )
		return false;

	    trcrg.limitTo( tkzs_.hsamp_.trcRange() );
	    tkzs_.zsamp_.limitTo( zrg );
	    tkzs_.hsamp_.setTrcRange( trcrg );
	}

	dp_ = new RegularSeisDataPack( SeisDataPack::categoryStr(true,false),
					&dc_);
	DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );
	dp_->setSampling( tkzs_ );
	dp_->setName( ioobj_->name() );
	if ( scaler_ && !scaler_->isEmpty() )
	    dp_->setScaler( *scaler_ );

	if ( !addComponents(*dp_,*ioobj_,components_,0) )
	    return false;
    }

    if ( !is2d_ )
    {
	PosInfo::CubeData cubedata;
	if ( rdr_.get3DGeometryInfo(cubedata) )
	    dp_->setTrcsSampling( new PosInfo::SortedCubeData(cubedata) );
    }

    totalnr_ = tkzs_.hsamp_.totalNr();
    nrdone_ = 0;

    mSetSelData()

    initialized_ = true;
    msg_ = uiStrings::phrReading( tr(" %1 \'%2\'").arg( uiStrings::sVolume() )
						  .arg( ioobj_->uiName() ) );
    return true;
}


bool SequentialReader::setDataPack( RegularSeisDataPack& dp, od_ostream* strm )
{
    dp_ = &dp;
    DPM( DataPackMgr::SeisID() ).add( dp_.ptr() );
    setDataChar( DataCharacteristics( dp.getDataDesc() ).userType() );
    setScaler( dp.getScaler() && !dp.getScaler()->isEmpty()
	       ? dp.getScaler()->clone() : 0 );
    //scaler_ won't be used with external dp, but setting it for consistency

    if ( dp.sampling().isDefined() )
	tkzs_ = dp.sampling();
    else
	dp_->setSampling( tkzs_ );

    if ( dp_->nrComponents() < components_.size() &&
	 !addComponents(*dp_,*ioobj_,components_,strm) )
	return false;

    return true;
}


int SequentialReader::nextStep()
{
    if ( !initialized_ && !init() )
	return ErrorOccurred();

    if ( Threads::WorkManager::twm().queueSize(queueid_) >
	 100*Threads::WorkManager::twm().nrThreads() )
	return MoreToDo();

    SeisTrc* trc = new SeisTrc;
    const int res = rdr_.get( trc->info() );
    if ( res==-1 )
    { delete trc; msg_ = rdr_.errMsg(); return ErrorOccurred(); }
    if ( res==0 ) { delete trc; return Finished(); }
    if ( res==2 ) { delete trc; return MoreToDo(); }

    if ( !rdr_.get(*trc) )
    { delete trc; msg_ = rdr_.errMsg(); return ErrorOccurred(); }

    const TypeSet<int>& outcomponents = outcomponents_
				      ? *outcomponents_ : components_;
    Task* task =
	new ArrayFiller( *trc, components_, outcomponents, *dp_, is2d_ );
    Threads::WorkManager::twm().addWork(
	Threads::Work(*task,true), 0, queueid_, false, false, true );

    nrdone_++;
    return MoreToDo();
}

} // namespace Seis
