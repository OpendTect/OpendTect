/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


#include "volprocvolreader.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisparallelreader.h"

namespace VolProc
{

VolumeReader::~VolumeReader()
{
    releaseData();
}


Task* VolumeReader::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !output || !ioobj )
	return 0;

    TypeSet<int> components;
    for ( int idx=0; idx<output->nrComponents(); idx++ )
	components += idx;

    Seis::SequentialReader* rdr = new Seis::SequentialReader( *ioobj, 0,
							      &components );
    if ( !rdr->setDataPack(*output) )
	{ delete rdr; return 0; }

    return rdr;
}


bool VolumeReader::setVolumeID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    return ioobj;
}


void VolumeReader::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    pars.set( sKeyVolumeID(), mid_ );
}


bool VolumeReader::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    MultiID mid;
    pars.get( sKeyVolumeID(), mid );
    return setVolumeID( mid );
}


od_int64 VolumeReader::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


} // namespace VolProc
