/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seisdatapackwriter.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "survinfo.h"


SeisDataPackWriter::SeisDataPackWriter( const MultiID& mid,
				  const RegularSeisDataPack& dp,
				  const TypeSet<int>& cubeindices )
    : Executor( "Attribute volume writer" )
    , nrdone_( 0 )
    , tks_( dp.sampling().hsamp_ )
    , zrg_( 0, dp.sampling().nrZ()-1 )
    , totalnr_( (int) dp.sampling().hsamp_.totalNr() )
    , cube_( dp )
    , iterator_( dp.sampling().hsamp_ )
    , mid_( mid )
    , writer_( 0 )
    , trc_( 0 )
    , cubeindices_( cubeindices )
{ 
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    delete trc_;
    delete writer_;
}    


od_int64 SeisDataPackWriter::nrDone() const
{ return nrdone_; }    


void SeisDataPackWriter::setSelection( const TrcKeySampling& hrg,
				    const Interval<int>& zrg )
{
    zrg_ = zrg;
    tks_ = hrg;

    iterator_.setSampling( hrg );
    totalnr_ = (int) hrg.totalNr();
}


od_int64 SeisDataPackWriter::totalNr() const
{ return totalnr_; }


int SeisDataPackWriter::nextStep()
{
    if ( !writer_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid_ );
	if ( !ioobj ) return ErrorOccurred(); 

	writer_ = new SeisTrcWriter( ioobj );

	const Interval<int> cubezrg( 0, cube_.sampling().nrZ()-1 );
	if ( !cubezrg.includes( zrg_.start,false ) ||
	     !cubezrg.includes( zrg_.stop,false ) )
	    zrg_ = cubezrg;

	const int trcsz = zrg_.width()+1;
	trc_ = new SeisTrc( trcsz );

	const float step = cube_.sampling().zsamp_.step;
	trc_->info().sampling.start = zrg_.start * step;
	trc_->info().sampling.step = step;
	trc_->info().nr = 0;

	for ( int idx=1; idx<cubeindices_.size(); idx++ )
	    trc_->data().addComponent( trcsz, DataCharacteristics() );

    }

    BinID currentpos;
    if ( !iterator_.next( currentpos ) )
	return Finished();

    const TrcKeySampling& hs = cube_.sampling().hsamp_;

    trc_->info().binid = currentpos;
    trc_->info().coord = SI().transform( currentpos );
    const int inlidx = hs.inlRange().nearestIndex( currentpos.inl() );
    const int crlidx = hs.crlRange().nearestIndex( currentpos.crl() );

    for ( int idx=0; idx<cubeindices_.size(); idx++ )
    {
	for ( int zidx=0; zidx<=zrg_.width(); zidx++ )
	{
	    const int zpos = zrg_.start+zidx;
	    const float value = 
		cube_.data(cubeindices_[idx]).get(inlidx, crlidx, zpos );
	    trc_->set( zidx, value, idx );
	}
    }
    
    if ( !writer_->put( *trc_ ) )
	return ErrorOccurred();

    nrdone_++;
    trc_->info().nr++;
    return MoreToDo();
}  

