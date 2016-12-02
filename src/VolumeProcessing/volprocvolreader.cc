/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


#include "volprocvolreader.h"

#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisloader.h"

namespace VolProc
{

class VolumeReaderExecutor : public SequentialTask
{ mODTextTranslationClass(VolumeReaderExecutor);
public:

VolumeReaderExecutor( const IOObj& ioobj, const TypeSet<int>& components,
		      const ObjectSet<Scaler>& compscalers,
		      RegularSeisDataPack& output )
    : SequentialTask()
    , ioobj_(ioobj.clone())
    , components_(components)
    , compscalers_(compscalers)
    , output_(output)
{}


~VolumeReaderExecutor()
{
    delete ioobj_;
}


uiString message() const
{ return msg_; }


protected:

#define mErrRet() \
{ \
    msg_ = rdr.message(); \
    return ErrorOccurred(); \
}

int nextStep()
{
    Seis::SequentialFSLoader rdr( *ioobj_, &output_.sampling(), &components_ );
    if ( !rdr.setDataPack(output_) )
	mErrRet()

    for ( int idx=0; idx<compscalers_.size(); idx++ )
    {
	if ( !compscalers_[idx] )
	    continue;

	rdr.setComponentScaler( *compscalers_[idx], idx );
    }

    rdr.setProgressMeter( progressmeter_ );
    if ( !rdr.execute() )
	mErrRet()

    return Finished();
}

private:

const IOObj*	ioobj_;
const TypeSet<int>&	components_;
const ObjectSet<Scaler>& compscalers_;
RegularSeisDataPack& output_;
uiString	msg_;

};


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

    const int initialdpnrcomp = output->nrComponents();
    const int nrcompdataset = compnms.size();
    if ( components_.isEmpty() )
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

	    components_ += idx;
	}
    }
    else
    {
	for ( int icomp=initialdpnrcomp; icomp<components_.size(); icomp++ )
	{
	    if ( !output->addComponent(0))
		return false;
	}

	for ( int icomp=0; icomp<components_.size(); icomp++ )
	{
	    const int compidx = components_[icomp];
	    if ( compidx >= nrcompdataset )
		return false;

	    output->setComponentName( compnms.get(compidx), icomp );
	}
    }

    return true;
}


VolumeReader::~VolumeReader()
{
    deepErase( compscalers_ );
}


Task* VolumeReader::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    PtrMan<IOObj> ioobj = DBM().get( mid_ );
    if ( !output || !ioobj || !prepareWork(*ioobj) )
    {
	Step::releaseData();
	return 0;
    }

    return new VolumeReaderExecutor( *ioobj, components_, compscalers_,
				     *output );
}


bool VolumeReader::setVolumeID( const DBKey& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = DBM().get( mid_ );
    return ioobj;
}


void VolumeReader::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    par.set( sKeyVolumeID(), mid_ );
    par.set( sKey::Component(), components_ );
    for ( int idx=0; idx<compscalers_.size(); idx++ )
    {
	if ( !compscalers_[idx] || compscalers_[idx]->isEmpty() )
	    continue;

	BufferString scalerstr;
	compscalers_[idx]->put( scalerstr.getCStr() );
	par.set( IOPar::compKey(sKey::Scale(),idx), scalerstr );
    }
}


bool VolumeReader::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKey::Component(),components_ );
    compscalers_.allowNull( true );
    PtrMan<IOPar> scalerpar = par.subselect( sKey::Scale() );
    if ( scalerpar.ptr() )
    {
	for ( int idx=0; idx<scalerpar->size(); idx++ )
	{
	    const BufferString compidxstr( scalerpar->getKey(idx) );
	    const int compidx = compidxstr.toInt();
	    BufferString scalerstr;
	    if ( !scalerpar->get(compidxstr,scalerstr) || scalerstr.isEmpty() )
		continue;

	    Scaler* scaler = Scaler::get( scalerstr );
	    if ( !scaler ) continue;

	    for ( int idy=0; idy<=compidx; idy++ )
	    {
		if ( !compscalers_.validIdx(idy) )
		    compscalers_ += 0;
	    }

	    delete compscalers_.replace( compidx, scaler );
	}
    }

    DBKey mid;
    return par.get( sKeyVolumeID(), mid ) ? setVolumeID( mid ) : true;
}


od_int64 VolumeReader::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


} // namespace VolProc
