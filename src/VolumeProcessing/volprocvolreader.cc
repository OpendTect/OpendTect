/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocvolreader.h"

#include "arrayndimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisparallelreader.h"

#include "hiddenparam.h"

HiddenParam<VolProc::VolumeReader,TypeSet<int>*> volproccompmgr_(0);

namespace VolProc
{

class VolumeReaderExecutor : public SequentialTask
{ mODTextTranslationClass(VolumeReaderExecutor);
public:

VolumeReaderExecutor( const IOObj& ioobj, const TypeSet<int>& components,
		      RegularSeisDataPack& output )
    : SequentialTask()
    , ioobj_(ioobj.clone())
    , components_(components)
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
    Seis::SequentialReader rdr( *ioobj_, &output_->sampling(), &components_ );
    if ( !rdr.setDataPack(*output_) )
	mErrRet()

    rdr.setProgressMeter( progressmeter_ );
    if ( !rdr.execute() )
	mErrRet()

    return Finished();
}

private:

const IOObj*	ioobj_;
const TypeSet<int>& components_;
RegularSeisDataPack* output_;
uiString	msg_;

};



VolumeReader::~VolumeReader()
{
    releaseData();
    if ( volproccompmgr_.hasParam( this ) )
    {
	delete volproccompmgr_.getParam( this );
	volproccompmgr_.removeParam( this );
    }
}


bool VolumeReader::prepareWork( const IOObj& ioobj )
{
    SeisIOObjInfo seisinfo( ioobj );
    if ( !seisinfo.isOK() )
	return false;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );

    BufferStringSet compnms;
    seisinfo.getComponentNames( compnms, output->sampling().hsamp_.getGeomID());
    if ( compnms.isEmpty() )
	return false;

    if ( !volproccompmgr_.hasParam(this) )
	volproccompmgr_.setParam( this, new TypeSet<int> );

    TypeSet<int>& components = *volproccompmgr_.getParam(this);

    const int initialdpnrcomp = output->nrComponents();
    const int nrcompdataset = compnms.size();
    if ( components.isEmpty() )
    {
	for ( int idx=0; idx<nrcompdataset; idx++ )
	{
	    if ( idx<initialdpnrcomp )
		output->setComponentName( compnms.get(idx), idx );
	    else
	    {
		if ( !output->addComponent(compnms.get(idx).str()) )
		    return false;
	    }

	    components += idx;
	}
    }
    else
    {
	for ( int icomp=initialdpnrcomp; icomp<components.size(); icomp++ )
	{
	    if ( !output->addComponent(0))
		return false;
	}

	for ( int icomp=0; icomp<components.size(); icomp++ )
	{
	    const int compidx = components[icomp];
	    if ( compidx >= nrcompdataset )
		return false;

	    output->setComponentName( compnms.get(compidx), icomp );
	}
    }

    return true;
}


Task* VolumeReader::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !output || !ioobj || !prepareWork(*ioobj) )
    {
	Step::releaseData();
	return 0;
    }

    TypeSet<int> emptycomponents;
    const TypeSet<int>& components = volproccompmgr_.hasParam( this )
				   ? *volproccompmgr_.getParam( this )
				   : emptycomponents;

    return new VolumeReaderExecutor( *ioobj, components, *output );
}


bool VolumeReader::setVolumeID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    return ioobj;
}


void VolumeReader::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    TypeSet<int> emptycomponents;
    const TypeSet<int>& components = volproccompmgr_.hasParam( this )
				   ? *volproccompmgr_.getParam( this )
				   : emptycomponents;

    par.set( sKeyVolumeID(), mid_ );
    par.set( sKey::Component(), components );
}


bool VolumeReader::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    if ( !volproccompmgr_.hasParam(this) )
	volproccompmgr_.setParam( this, new TypeSet<int> );

    par.get( sKey::Component(), *volproccompmgr_.getParam( this ) );

    MultiID mid;
    return par.get( sKeyVolumeID(), mid ) ? setVolumeID( mid ) : true;
}


od_int64 VolumeReader::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


} // namespace VolProc
