/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisparallelreader.h"

#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "binidvalset.h"
#include "cbvsreadmgr.h"
#include "ioobj.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seis2ddata.h"

namespace Seis
{

ParallelReader::ParallelReader( const IOObj& ioobj,
	const TypeSet<int>& components,
	const ObjectSet<Array3D<float> >& arrays,
	const TrcKeyZSampling& cs )
    : arrays_( new ObjectSet<Array3D<float> >( arrays ) )
    , components_( components )
    , bidvals_( 0 )
    , cs_( cs )
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hrg.totalNr() )
{}


ParallelReader::ParallelReader( const IOObj& ioobj, const TrcKeyZSampling& cs )
    : arrays_(new ObjectSet<Array3D<float> >)
    , bidvals_(0)
    , cs_(cs)
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hrg.totalNr() )
{
    SeisIOObjInfo seisinfo( ioobj );
    const int nrcomponents = seisinfo.nrComponents();
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;
}


ParallelReader::ParallelReader( const IOObj& ioobj,
                      BinIDValueSet& bidvals,
		      const TypeSet<int>& components )
    : arrays_( 0 )
    , components_( components )
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{}


ParallelReader::~ParallelReader()
{
    delete arrays_;
    delete ioobj_;
}


uiString ParallelReader::uiNrDoneText() const
{ return "Traces read"; }


uiString ParallelReader::uiMessage() const
{ return errmsg_.isEmpty() ? "Reading" : errmsg_; }


bool ParallelReader::doPrepare( int nrthreads )
{
    const char* allocprob = "Cannot allocate memory";

    if ( bidvals_ )
    {
	pErrMsg("The bidval-code is not tested. Run through step by step, make "
		"sure everything is OK and remove this warning.");
	const int nrvals = 1+components_.size();
        if ( bidvals_->nrVals()!=nrvals )
	{
	    if ( !bidvals_->setNrVals( nrvals, true ) )
	    {
		errmsg_ = allocprob;
		return false;
	    }
	}
    }
    else
    {
        for ( int idx=0; idx<components_.size(); idx++ )
        {
	    const Array3DInfoImpl sizes( cs_.hrg.nrInl(), cs_.hrg.nrCrl(),
				     cs_.zrg.nrSteps()+1 );
	    bool setbg = false;
            if ( idx>=arrays_->size() )
            {
		mDeclareAndTryAlloc( Array3D<float>*, arr,
				     Array3DImpl<float>( sizes ) );
		if ( !arr || !arr->isOK() )
		{
		    errmsg_ = allocprob;
		    return false;
		}

                (*arrays_) +=  arr;
		setbg = true;
            }
	    else
	    {
		if ( (*arrays_)[idx]->info()!=sizes )
		{
		    if ( !(*arrays_)[idx]->setInfo( sizes ) )
		    {
			errmsg_ = allocprob;
			return false;
		    }

		    setbg = true;
		}
	    }

	    if ( setbg )
		(*arrays_)[idx]->setAll( mUdf(float) );
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
	errmsg_ = "Cannot open storage";
	return false;
    }

    if ( !reader->prepareWork() )
    {
	errmsg_ = reader->errMsg().getFullString();
	return false;
    }

    //Set ranges, at least z-range
    mDynamicCastGet( SeisTrcTranslator*, translator,
		     reader->translator() );

    if ( !translator || !translator->supportsGoTo() )
    {
	errmsg_ = "Storage does not support random access";
	return false;
    }

    TrcKeySamplingIterator iter;
    BinIDValueSet::SPos bidvalpos;
    BinID curbid;
    if ( bidvals_ )
    {
        bidvalpos = bidvals_->getPos( start );
        if ( !bidvalpos.isValid() )
            return false;

        curbid = bidvals_->getBinID( bidvalpos );
    }
    else
    {
        iter.setSampling( cs_.hrg );
	iter.setNextPos( cs_.hrg.atIndex( start ) );
	iter.next( curbid );
    }

    SeisTrc trc;

#define mUpdateInterval 100
    int nrdone = 0;
    for ( od_int64 idx=start; true; idx++, nrdone++ )
    {
	if ( nrdone>mUpdateInterval )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;

	    if ( !shouldContinue() )
		return false;
	}

        if ( translator->goTo( curbid ) && reader->get( trc ) &&
            trc.info().binid==curbid )
        {
	    const StepInterval<float> trczrg = trc.zRange();

            if ( bidvals_ )
            {
		float* vals = bidvals_->getVals(bidvalpos);
		const float z = vals[0];

		if ( !mIsUdf(z) && trczrg.includes( z, false ) )
		{
		    for ( int idc=components_.size()-1; idc>=0; idc-- )
		    {
			vals[idc+1] = trc.getValue( z, components_[idc] );
		    }
		}
            }
            else
            {
		const int inlidx = cs_.hrg.inlIdx( curbid.inl() );
		const int crlidx = cs_.hrg.crlIdx( curbid.crl() );

		for ( int idz=(*arrays_)[0]->info().getSize(2)-1; idz>=0; idz--)
		{
		    float val;
		    const double z = cs_.zrg.atIndex( idz );
		    if ( trczrg.includes( z, false ) )
		    {
			for ( int idc=arrays_->size()-1; idc>=0; idc-- )
			{
			    val = trc.getValue( (float) z, components_[idc] );
			    if ( !mIsUdf(val) )
			    {
				(*arrays_)[idc]->set( inlidx, crlidx, idz, val);
			    }
			}
		    }
		}
            }
        }

        if ( idx==stop )
            break;

        if ( bidvals_ )
        {
            if ( !bidvals_->next(bidvalpos,false) )
                return false;

            curbid = bidvals_->getBinID( bidvalpos );
        }
        else
        {
            if ( !iter.next( curbid ) )
                return false;
        }
    }

    addToNrDone( nrdone );

    return true;
}


bool ParallelReader::doFinish( bool success )
{ return success; }



// ParallelReader2D
ParallelReader2D::ParallelReader2D( const IOObj& ioobj, Pos::GeomID geomid,
				    const TrcKeyZSampling& cs )
    : arrays_(new ObjectSet<Array2D<float> >)
    , geomid_(geomid)
    , cs_(cs)
    , ioobj_(ioobj.clone())
    , totalnr_(cs.hrg.nrCrl())
{
    SeisIOObjInfo seisinfo( ioobj );
    const int nrcomponents = seisinfo.nrComponents();
    for ( int idx=0; idx<nrcomponents; idx++ )
	components_ += idx;
}


ParallelReader2D::~ParallelReader2D()
{
    delete arrays_;
    delete ioobj_;
}


uiString ParallelReader2D::uiNrDoneText() const
{ return "Traces read"; }

uiString ParallelReader2D::uiMessage() const
{ return errmsg_.isEmpty() ? "Reading" : errmsg_; }

od_int64 ParallelReader2D::nrIterations() const
{ return totalnr_; }


bool ParallelReader2D::doPrepare( int nrthreads )
{
    const uiString allocprob = tr("Cannot allocate memory");

    for ( int idx=0; idx<components_.size(); idx++ )
    {
	const Array2DInfoImpl sizes( cs_.nrCrl(), cs_.nrZ() );
	bool setbg = false;
	if ( idx>=arrays_->size() )
	{
	    mDeclareAndTryAlloc( Array2D<float>*, arr,
				 Array2DImpl<float>( sizes ) );
	    if ( !arr || !arr->isOK() )
	    {
		errmsg_ = allocprob;
		return false;
	    }

	    (*arrays_) +=  arr;
	    setbg = true;
	}
	else
	{
	    if ( (*arrays_)[idx]->info()!=sizes )
	    {
		if ( !(*arrays_)[idx]->setInfo(sizes) )
		{
		    errmsg_ = allocprob;
		    return false;
		}

		setbg = true;
	    }
	}

	if ( setbg )
	    (*arrays_)[idx]->setAll( mUdf(float) );
    }

    return true;
}


bool ParallelReader2D::doWork( od_int64 start, od_int64 stop, int threadid )
{
    PtrMan<IOObj> ioobj = ioobj_->clone();
    const Seis2DDataSet dataset( *ioobj );
    const int lidx = dataset.indexOf( geomid_ );
    if ( lidx<0 ) return false;

    const IOPar& iopar = dataset.getInfo( lidx );
    const char* fnm = SeisCBVS2DLineIOProvider::getFileName( iopar );
    PtrMan<CBVSSeisTrcTranslator> trl =
	CBVSSeisTrcTranslator::make( fnm, false, true );
    if ( !trl ) return false;

    SeisTrc trc;
    BinID curbid;
    StepInterval<int> trcrg = cs_.hrg.crlRange();
    trl->toStart();
    curbid = trl->readMgr()->binID();

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	curbid.crl() = trcrg.atIndex( mCast(int,idx) );
	if ( trl->goTo(curbid) && trl->read(trc) )
	{
	    const StepInterval<float> trczrg = trc.zRange();
	    for ( int idz=(*arrays_)[0]->info().getSize(1)-1; idz>=0; idz--)
	    {
		float val;
		const float z = cs_.zrg.atIndex( idz );
		if ( trczrg.includes(z,false) )
		{
		    for ( int idc=arrays_->size()-1; idc>=0; idc-- )
		    {
			val = trc.getValue( z, components_[idc] );
			if ( !mIsUdf(val) )
			{
			    (*arrays_)[idc]->set( mCast(int,idx), idz, val );
			}
		    }
		}
	    }
	}

	addToNrDone( 1 );
    }

    return true;
}


bool ParallelReader2D::doFinish( bool success )
{ return success; }

} // namespace Seis
