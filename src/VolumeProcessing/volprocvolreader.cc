/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


#include "volprocvolreader.h"

#include "ioobj.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisloader.h"
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
    , output_(output)
{
    output.setName( "Input Data" );
    adjustSteeringScaler();
}


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
    Seis::SequentialFSLoader rdr( *ioobj_, &output_.subSel(), &components_ );
    if ( !rdr.setDataPack(output_) )
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
    if ( !output_.is2D() || !ioobj_ )
	return;

    BufferString type;
    if ( !ioobj_->pars().get(sKey::Type(),type) ||
	 type != BufferString(sKey::Steering()) )
	return;

    const auto& geom2d = SurvGeom::get2D( output_.subSel().geomID() );
    if ( geom2d.isEmpty() )
	return;

    double trcdist = geom2d.averageTrcDist();
    const UnitOfMeasure* feetuom = UnitOfMeasure::feetUnit();
    if ( feetuom && SI().xyInFeet() )
	trcdist = feetuom->getSIValue( trcdist );

    double zstep = output_.zRange().step;
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const ZDomain::Def& zdef = SI().zDomain();
    if ( zuom && zdef.isDepth() )
	zstep = zuom->getSIValue( zstep );

    const UnitOfMeasure* zdipuom = zdef.isDepth() ? UoMR().get( "Millimeters" )
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
const TypeSet<int>&	components_;
const ObjectSet<Scaler>& compscalers_;
RegularSeisDataPack& output_;
uiString	msg_;

};



VolumeReader::~VolumeReader()
{
    deepErase( compscalers_ );
}


bool VolumeReader::prepareWork( int )
{
    if ( !Step::prepareWork() )
	return false;

    PtrMan<IOObj> ioobj = mid_.getIOObj();
    if ( !ioobj )
	return false;

    SeisIOObjInfo seisinfo( ioobj );
    if ( !seisinfo.isOK() )
	return false;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    BufferStringSet compnms;
    seisinfo.getComponentNames( compnms, output->subSel().geomID());
    if ( compnms.isEmpty() )
	return false;

    const int nrcomps = output->nrComponents();
    if ( components_.isEmpty() )
    {
	for ( int idx=0; idx<nrcomps; idx++ )
	{
	    output->setComponentName( compnms.get(idx), idx );
	    components_ += idx;
	}
    }
    else
    {
	for ( int icomp=0; icomp<components_.size(); icomp++ )
	{
	    const int compidx = components_[icomp];
	    output->setComponentName( compnms.get(compidx), icomp );
	}
    }

    return true;
}


ReportingTask* VolumeReader::createTask()
{
    if ( !prepareWork() )
	return 0;

    PtrMan<IOObj> ioobj = mid_.getIOObj();
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );

    return new VolumeReaderExecutor( *ioobj, components_, compscalers_,
				     *output );
}


bool VolumeReader::setVolumeID( const DBKey& mid )
{
    mid_ = mid;
    PtrMan<IOObj> ioobj = mid_.getIOObj();
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
    compscalers_.setNullAllowed( true );
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


int VolumeReader::getNrOutComponents( OutputSlotID slotid,
				      Pos::GeomID geomid ) const
{
    if ( !validOutputSlotID(slotid) )
	return Step::getNrOutComponents( slotid, geomid );

    if ( components_.size() > 0 )
	return components_.size();

    PtrMan<IOObj> ioobj = mid_.getIOObj();
    SeisIOObjInfo seisinfo( ioobj );
    if ( !seisinfo.isOK() )
	return Step::getNrOutComponents( slotid, geomid );

    return seisinfo.nrComponents( geomid );
}

} // namespace VolProc
