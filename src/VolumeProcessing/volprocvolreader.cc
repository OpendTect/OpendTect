/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocvolreader.h"

#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "survgeom2d.h"
#include "unitofmeasure.h"


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
    , output_(&output)
{
    output.setName( "Input Data" );
    adjustSteeringScaler();
}


~VolumeReaderExecutor()
{
    delete ioobj_;
}


uiString uiMessage() const override
{ return msg_; }


protected:

#define mErrRet() \
{ \
    msg_ = rdr.uiMessage(); \
    return ErrorOccurred(); \
}

int nextStep() override
{
    Seis::SequentialReader rdr( *ioobj_, &output_->sampling(), &components_ );
    if ( !rdr.setDataPack(*output_) )
	mErrRet()

    for ( int idx=0; idx<compscalers_.size(); idx++ )
    {
	if ( !compscalers_[idx] )
	    continue;

	rdr.setComponentScaler( *compscalers_[idx], idx );
    }

    rdr.getProgress( *this );
    if ( !rdr.execute() )
	mErrRet()

    return Finished();
}

private:

void adjustSteeringScaler()
{
    if ( !output_ || !output_->is2D() || !ioobj_ )
	return;

    BufferString type;
    if ( !ioobj_->pars().get(sKey::Type(),type) ||
	 type != BufferString(sKey::Steering()) )
	return;

    const Survey::Geometry* geom = Survey::GM().getGeometry(
				   output_->sampling().hsamp_.getGeomID() );
    if ( !geom || !geom->as2D() )
	return;

    double trcdist = geom->as2D()->averageTrcDist();
    const UnitOfMeasure* feetuom = UoMR().get( "Feet" );
    if ( feetuom && SI().xyInFeet() )
	trcdist = feetuom->getSIValue( trcdist );

    double zstep = output_->sampling().zsamp_.step_;
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const ZDomain::Def& zdef = SI().zDomain();
    if ( zuom && zdef.isDepth() )
	zstep = zuom->getSIValue( zstep );

    const UnitOfMeasure* zdipuom = zdef.isDepth() ? UoMR().get( "Millimeter" )
						  : UoMR().get( "Microseconds");
    const UnitOfMeasure* targetzuom = zdipuom;
    if ( targetzuom )
	zstep = targetzuom->getUserValueFromSI( zstep );

    const double scalefactor = trcdist / zstep;
    const LinScaler dipscaler( 0., scalefactor );
    ObjectSet<Scaler>& compscalersedit =
				const_cast<ObjectSet<Scaler>&>( compscalers_ );
    for ( int compidx=0; compidx<compscalers_.size(); compidx++ )
    {
	if ( !compscalers_[compidx] || compscalers_[compidx]->isEmpty() )
	    return;

	delete compscalersedit.replace( compidx, dipscaler.clone() );
    }
}

const IOObj*	ioobj_;
const TypeSet<int>& components_;
const ObjectSet<Scaler>& compscalers_;
RegularSeisDataPack* output_;
uiString	msg_;

};



VolumeReader::~VolumeReader()
{
    releaseData();
    deepErase( compscalers_ );
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
		if ( !output->addComponentNoInit(compnms.get(idx).str()) )
		    return false;
	    }

	    components_ += idx;
	}
    }
    else
    {
	for ( int icomp=initialdpnrcomp; icomp<components_.size(); icomp++ )
	{
	    if ( !output->addComponentNoInit(0))
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


Task* VolumeReader::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !output || !ioobj || !prepareWork(*ioobj) )
    {
	Step::releaseData();
	return 0;
    }

    return new VolumeReaderExecutor( *ioobj, components_, compscalers_,
				     *output );
}


bool VolumeReader::setVolumeID( const MultiID& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj )
    {
	errmsg_ = uiStrings::phrCannotFindDBEntry( mid_ );
	return false;
    }

    int nrcomps = components_.size();
    if ( nrcomps == 0 )
    {
	SeisIOObjInfo seisinfo( ioobj.ptr() );
	if ( !seisinfo.isOK() )
	    return false;

	BufferStringSet compnms;
	seisinfo.getComponentNames( compnms );
	nrcomps = compnms.isEmpty() ? 1 : compnms.size();
    }

    setOutputNrComps( nrcomps );

    return true;
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

	BufferString scalerstr( 256, false );
	compscalers_[idx]->put( scalerstr.getCStr(), scalerstr.bufSize() );
	par.set( IOPar::compKey(sKey::Scale(),idx), scalerstr );
    }
}


bool VolumeReader::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    par.get( sKey::Component(), components_ );
    compscalers_.allowNull( true );
    PtrMan<IOPar> scalerpar = par.subselect( sKey::Scale() );
    if ( scalerpar )
    {
	IOParIterator iter( *scalerpar );
	BufferString compidxstr, scalerstr;
	while ( iter.next(compidxstr,scalerstr) )
	{
	    const int compidx = compidxstr.toInt();
	    if ( scalerstr.isEmpty() )
		continue;

	    Scaler* scaler = Scaler::get( scalerstr );
	    if ( !scaler )
		continue;

	    while ( compscalers_.size() <= compidx )
		compscalers_ += nullptr;

	    delete compscalers_.replace( compidx, scaler );
	}
    }

    MultiID mid;
    return par.get( sKeyVolumeID(), mid ) ? setVolumeID( mid ) : true;
}


od_int64 VolumeReader::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling&, const StepInterval<int>& ) const
{
    return 0;
}


} // namespace VolProc
