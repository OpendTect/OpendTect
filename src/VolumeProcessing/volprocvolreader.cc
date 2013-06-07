/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID = "$Id$";

#include "volprocvolreader.h"

#include "arraynd.h"
#include "ioman.h"
#include "ioobj.h"
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
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj )
	return 0;

    TypeSet<int> components( 1, 0 );
    ObjectSet<Array3D<float> > arrays;
    arrays += &output_->getCube(0);

    return new Seis::ParallelReader( *ioobj, components, arrays,
				     output_->cubeSampling() );
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


}; //namespace
