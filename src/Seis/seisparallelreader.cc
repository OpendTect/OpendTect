/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID = "$Id: seisparallelreader.cc,v 1.1 2011-08-10 06:15:41 cvskris Exp $";

#include "seisparallelreader.h"

#include "arrayndimpl.h"
#include "ioobj.h"
#include "cubesampling.h"
#include "binidvalset.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"

Seis::ParallelReader::ParallelReader( const IOObj& ioobj,
	const TypeSet<int>& components, ObjectSet<Array3D<float> >& arrays,
	const CubeSampling& cs )
    : arrays_( &arrays )
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
    delete ioobj_;
}


bool Seis::ParallelReader::doPrepare( int nrthreads )
{
    if ( bidvals_ )
    {
        if ( bidvals_->nrVals()!=1+components_.size() )
	        bidvals_->setNrVals( 1+components_.size(), true );
    }
    else
    {
        for ( int idx=0; idx<components_.size(); idx++ )
        {
            if ( idx>=arrays_->size() )
            {
                (*arrays_) += new Array3DImpl<float>( cs_.hrg.nrInl(),
                                                   cs_.hrg.nrCrl(),
                                                   cs_.zrg.nrSteps() );
            }
        }
    }
    
    return true;
}


bool Seis::ParallelReader::doWork( od_int64 start, od_int64 stop, int threadid )
{
    PtrMan<SeisTrcReader> reader = new SeisTrcReader( ioobj_ );
    if ( !reader || !reader->prepareWork() )
    {
	return false;
    }
    //Set ranges, at least z-range

    mDynamicCastGet( SeisTrcTranslator*, translator,
		     reader->translator() );

    if ( !translator || !translator->supportsGoTo() )
    {
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
    }

    SeisTrc trc;

    for ( od_int64 idx=start; true; idx++ )
    {
        if ( translator->goTo( curbid ) && reader->get( trc ) &&
            trc.info().binid==curbid )
        {
            if ( bidvals_ )
            {
            }
            else
            {
            }
        }
        else
        {
            if ( bidvals_ )
            {
            }
            else
            {
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
    
    return true;
}


bool Seis::ParallelReader::doFinish( bool success )
{ return success; }
	    


