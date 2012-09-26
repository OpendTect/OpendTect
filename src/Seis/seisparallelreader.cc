/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisparallelreader.h"

#include "arrayndimpl.h"
#include "ioobj.h"
#include "cubesampling.h"
#include "binidvalset.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"

Seis::ParallelReader::ParallelReader( const IOObj& ioobj,
	const TypeSet<int>& components,
	const ObjectSet<Array3D<float> >& arrays,
	const CubeSampling& cs )
    : arrays_( new ObjectSet<Array3D<float> >( arrays ) )
    , components_( components )
    , bidvals_( 0 )
    , cs_( cs )
    , ioobj_( ioobj.clone() )
    , totalnr_( cs.hrg.totalNr() )
{}


Seis::ParallelReader::ParallelReader( const IOObj& ioobj,
                      BinIDValueSet& bidvals,
		      const TypeSet<int>& components )
    : arrays_( 0 )
    , components_( components )
    , bidvals_( &bidvals )
    , ioobj_( ioobj.clone() )
    , totalnr_( bidvals.totalSize() )
{}


Seis::ParallelReader::~ParallelReader()
{
    delete arrays_;
    delete ioobj_;
}


bool Seis::ParallelReader::doPrepare( int nrthreads )
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


bool Seis::ParallelReader::doWork( od_int64 start, od_int64 stop, int threadid )
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
	errmsg_ = reader->errMsg();
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

    HorSamplingIterator iter;
    BinIDValueSet::Pos bidvalpos;
    BinID curbid;
    if ( bidvals_ )
    {
        bidvalpos = bidvals_->getPos( start );
        if ( !bidvalpos.valid() )
            return false;
        
        curbid = bidvals_->getBinID( bidvalpos );
    }
    else
    {
        iter.setSampling( cs_.hrg );
        curbid = cs_.hrg.atIndex( start );
	iter.reset( false );
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
		const int inlidx = cs_.hrg.inlIdx( curbid.inl );
		const int crlidx = cs_.hrg.crlIdx( curbid.crl );

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


bool Seis::ParallelReader::doFinish( bool success )
{ return success; }
	    


