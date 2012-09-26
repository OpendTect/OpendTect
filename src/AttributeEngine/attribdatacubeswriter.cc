/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "attribdatacubeswriter.h"

#include "attribdatacubes.h"
#include "arraynd.h"
#include "ioman.h"
#include "ioobj.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "survinfo.h"

namespace Attrib
{ 


DataCubesWriter::DataCubesWriter( const MultiID& mid,
				  const Attrib::DataCubes& dc,
				  const TypeSet<int>& cubeindices )
    : Executor( "Attribute volume writer" )
    , nrdone_( 0 )
    , hrg_( dc.cubeSampling().hrg )
    , zrg_( dc.z0_, dc.z0_+dc.getZSz()-1 )
    , totalnr_( (int) dc.cubeSampling().hrg.totalNr() )
    , cube_( dc )
    , iterator_( dc.cubeSampling().hrg )
    , currentpos_( 0, 0 )
    , mid_( mid )
    , writer_( 0 )
    , trc_( 0 )
    , cubeindices_( cubeindices )
{ 
    cube_.ref();
}


DataCubesWriter::~DataCubesWriter()
{
    cube_.unRef();
    delete trc_;
    delete writer_;
}    


od_int64 DataCubesWriter::nrDone() const
{ return nrdone_; }    


void DataCubesWriter::setSelection( const HorSampling& hrg,
				    const Interval<int>& zrg )
{
    zrg_ = zrg;
    hrg_ = hrg;

    iterator_.setSampling( hrg );
    totalnr_ = (int) hrg.totalNr();
}


od_int64 DataCubesWriter::totalNr() const
{ return totalnr_; }


int DataCubesWriter::nextStep()
{
    if ( !writer_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid_ );
	if ( !ioobj ) return ErrorOccurred(); 

	writer_ = new SeisTrcWriter( ioobj );

	const Interval<int> cubezrg( cube_.z0_, cube_.z0_+cube_.getZSz()-1 );
	if ( !cubezrg.includes( zrg_.start,false ) ||
	     !cubezrg.includes( zrg_.stop,false ) )
	    zrg_ = cubezrg;

	const int trcsz = zrg_.width()+1;
	trc_ = new SeisTrc( trcsz );

	trc_->info().sampling.start = (float) (zrg_.start * cube_.zstep_);
	trc_->info().sampling.step = (float) cube_.zstep_;
	trc_->info().nr = 0;

	for ( int idx=1; idx<cubeindices_.size(); idx++ )
	    trc_->data().addComponent( trcsz, DataCharacteristics() );

    }

    if ( !iterator_.next( currentpos_ ) )
	return Finished();

    trc_->info().binid = currentpos_;
    trc_->info().coord = SI().transform( currentpos_ );
    const int inlidx = cube_.inlsampling_.nearestIndex( currentpos_.inl );
    const int crlidx = cube_.crlsampling_.nearestIndex( currentpos_.crl );

    for ( int idx=0; idx<cubeindices_.size(); idx++ )
    {
	for ( int zidx=0; zidx<cube_.getZSz(); zidx++ )
	{
	    const int zpos = zrg_.start+zidx;
	    const float value = cube_.getCube(cubeindices_[idx]).get(
					      inlidx, crlidx, zpos-cube_.z0_ );
	    trc_->set( zidx, value, idx );
	}
    }
    
    if ( !writer_->put( *trc_ ) )
	return ErrorOccurred();

    nrdone_++;
    trc_->info().nr++;
    return MoreToDo();
}  

}; //namespace
