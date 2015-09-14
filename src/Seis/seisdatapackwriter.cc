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
#include "seistrctr.h"
#include "survinfo.h"


SeisDataPackWriter::SeisDataPackWriter( const MultiID& mid,
				  const RegularSeisDataPack& dp,
				  const TypeSet<int>& cubeindices )
    : Executor( "Attribute volume writer" )
    , nrdone_( 0 )
    , tks_( dp.sampling().hsamp_ )
    , totalnr_( (int) dp.sampling().hsamp_.totalNr() )
    , cube_( dp )
    , iterator_( dp.sampling().hsamp_ )
    , mid_( mid )
    , writer_( 0 )
    , trc_( 0 )
    , cubeindices_( cubeindices )
{
    const int startz =
	mNINT32(dp.sampling().zsamp_.start/dp.sampling().zsamp_.step);
    zrg_ = Interval<int>( startz, startz+dp.sampling().nrZ()-1 );
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
    const int cubestartz =
	mNINT32(cube_.sampling().zsamp_.start/cube_.sampling().zsamp_.step);
    const Interval<int> cubezrg( cubestartz,
				 cubestartz+cube_.sampling().nrZ()-1 );

    if ( !writer_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid_ );
	if ( !ioobj || cube_.isEmpty() )
	    return ErrorOccurred();

	writer_ = new SeisTrcWriter( ioobj );


	const int trcsz = zrg_.width()+1;
	trc_ = new SeisTrc( trcsz );

	const float step = cube_.sampling().zsamp_.step;
	trc_->info().sampling.start = zrg_.start * step;
	trc_->info().sampling.step = step;
	trc_->info().nr = 0;

	BufferStringSet compnames;
	compnames.add( cube_.getComponentName() );
	for ( int idx=1; idx<cubeindices_.size(); idx++ )
	{
	    trc_->data().addComponent( trcsz, DataCharacteristics() );
	    compnames.add( cube_.getComponentName(idx) );
	}

	SeisTrcTranslator* transl = writer_->seisTranslator();
	if ( transl ) transl->setComponentNames( compnames );
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
	    const int zsample = zidx+zrg_.start;
	    const int cubesample = zsample - cubestartz;

	    const float value = cubezrg.includes( zsample, false )
		? cube_.data(cubeindices_[idx]).get(inlidx,crlidx,cubesample)
		: mUdf(float);

	    trc_->set( zidx, value, idx );
	}
    }

    if ( !writer_->put( *trc_ ) )
	return ErrorOccurred();

    nrdone_++;
    trc_->info().nr++;
    return MoreToDo();
}

