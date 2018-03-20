/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "seisblocksbackend.h"
#include "seismemblocks.h"
#include "od_iostream.h"
#include "uistrings.h"
#include "hdf5reader.h"
#include "hdf5writer.h"


Seis::Blocks::HDF5WriteBackEnd::HDF5WriteBackEnd( Writer& wrr )
    : WriteBackEnd(wrr)
{
}


Seis::Blocks::HDF5WriteBackEnd::~HDF5WriteBackEnd()
{
    //TODO add file-level info before close
}


void Seis::Blocks::HDF5WriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
}


void Seis::Blocks::HDF5WriteBackEnd::putBlock( int icomp, MemBlock& block,
		    HLocIdx wrstart, HDimensions wrhdims, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
}


Seis::Blocks::HDF5ReadBackEnd::HDF5ReadBackEnd( Reader& rdr, const char* fnm,
						uiRetVal& uirv )
    : ReadBackEnd(rdr)
{
}


Seis::Blocks::HDF5ReadBackEnd::~HDF5ReadBackEnd()
{
}


void Seis::Blocks::HDF5ReadBackEnd::reset( const char* fnm, uiRetVal& uirv )
{
    if ( !HDF5::isAvailable() )
	{ uirv.add( HDF5::Access::sHDF5NotAvailable(fnm) ); return; }
    uirv.add( mTODONotImplPhrase() );
}


Seis::Blocks::Column* Seis::Blocks::HDF5ReadBackEnd::createColumn(
				const HGlobIdx& gidx, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
    return 0;
}


void Seis::Blocks::HDF5ReadBackEnd::fillTrace( Column& column, const BinID& bid,
				SeisTrc& trc, uiRetVal& uirv ) const
{
    uirv.add( mTODONotImplPhrase() );
}
