/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


#include "volprocvolreader.h"

#include "dbman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisparallelreader.h"

namespace VolProc
{

class VolumeReaderExecutor : public SequentialTask
{ mODTextTranslationClass(VolumeReaderExecutor);
public:

VolumeReaderExecutor( const IOObj& ioobj, RegularSeisDataPack& output )
    : SequentialTask()
    , ioobj_(ioobj.clone())
    , output_(&output)
{}


~VolumeReaderExecutor()
{
    delete ioobj_;
}


uiString uiMessage() const
{ return msg_; }


protected:

#define mErrRet() \
{ \
    msg_ = rdr.uiMessage(); \
    return ErrorOccurred(); \
}

int nextStep()
{
    TypeSet<int> components;
    for ( int idx=0; idx<output_->nrComponents(); idx++ )
	components += idx;

    Seis::SequentialReader rdr( *ioobj_, 0, &components );
    if ( !rdr.setDataPack(*output_) )
	mErrRet()

    if ( progressmeter_ )
    {
	progressmeter_->setMessage( rdr.uiMessage() );
	rdr.setProgressMeter( progressmeter_ );
    }

    if ( !rdr.execute() )
	mErrRet()

    output_ = 0; //This executor no longer needs the output (the step has it).

    return Finished();
}

private:

const IOObj*	ioobj_;
RefMan<RegularSeisDataPack> output_;
uiString	msg_;

};


Task* VolumeReader::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    PtrMan<IOObj> ioobj = DBM().get( mid_ );
    if ( !output || !ioobj )
	return 0;

    return new VolumeReaderExecutor( *ioobj, *output );
}


bool VolumeReader::setVolumeID( const DBKey& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = DBM().get( mid_ );
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

    DBKey mid;
    pars.get( sKeyVolumeID(), mid );
    return setVolumeID( mid );
}


od_int64 VolumeReader::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


} // namespace VolProc
