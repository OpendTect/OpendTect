/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID = "$Id: attribdatacubeswriter.cc,v 1.6 2010-04-20 22:03:25 cvskris Exp $";

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
    , totalnr_( dc.cubeSampling().totalNr() )
    , cube_( dc )
    , iterator_( dc.cubeSampling().hrg )
    , currentpos_( 0 )
    , mid_( mid )
    , writer_( 0 )
    , trc_( *new SeisTrc( dc.getZSz() ) )
    , cubeindices_( cubeindices )
{ 
    cube_.ref();
    const CubeSampling cs = cube_.cubeSampling();

    trc_.info().sampling.start = cs.zrg.start;
    trc_.info().sampling.step = cs.zrg.step;
    trc_.info().nr = 0;

    for ( int idx=1; idx<cubeindices_.size(); idx++ )
	trc_.data().addComponent( dc.getZSz(), DataCharacteristics() );
}


DataCubesWriter::~DataCubesWriter()
{
    cube_.unRef();
    delete &trc_;
    delete writer_;
}    


od_int64 DataCubesWriter::nrDone() const
{ return nrdone_; }    


od_int64 DataCubesWriter::totalNr() const
{ return totalnr_; }


int DataCubesWriter::nextStep()
{
    if ( !writer_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid_ );
	if ( !ioobj ) return ErrorOccurred(); 

	writer_ = new SeisTrcWriter( ioobj );
    }

    if ( !iterator_.next( currentpos_ ) )
	return Finished();

    trc_.info().binid = currentpos_;
    trc_.info().coord = SI().transform( currentpos_ );
    const int inlidx = cube_.inlsampling_.nearestIndex( currentpos_.inl );
    const int crlidx = cube_.crlsampling_.nearestIndex( currentpos_.crl );

    for ( int idx=0; idx<cubeindices_.size(); idx++ )
    {
	for ( int zidx=0; zidx<cube_.getZSz(); zidx++ )
	{
	    const float value =
		cube_.getCube(cubeindices_[idx]).get( inlidx, crlidx, zidx );
	    trc_.set( zidx, value, idx );
	}
    }
    
    if ( !writer_->put( trc_ ) )
	return ErrorOccurred();

    nrdone_++;
    trc_.info().nr++;
    return MoreToDo();
}  

}; //namespace
