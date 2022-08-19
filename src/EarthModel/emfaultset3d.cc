/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfaultset3d.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "ioman.h"
#include "randcolor.h"

namespace EM {

mImplementEMObjFuncs( FaultSet3D, EMFaultSet3DTranslatorGroup::sGroupName() )


FaultSet3D::FaultSet3D( EMManager& man )
    : EMObject(man)
{
    setPreferredColor( OD::getRandomColor() );
    setPreferredMarkerStyle3D(
	MarkerStyle3D(MarkerStyle3D::Cube,3,OD::Color::Yellow()) );
}


FaultSet3D::~FaultSet3D()
{}


Executor* FaultSet3D::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return nullptr;

    PtrMan<EMFaultSet3DTranslator> transl =
		sCast(EMFaultSet3DTranslator*,ioobj->createTranslator());
    return transl ? transl->reader( *this, *ioobj ) : nullptr;
}


const IOObjContext& FaultSet3D::getIOObjContext() const
{
    return EMFaultSet3DTranslatorGroup::ioContext();
}


uiString FaultSet3D::getUserTypeStr() const
{
    return EMFaultSet3DTranslatorGroup::sTypeName();
}


int FaultSet3D::nrFaults() const
{
    return faults_.size();
}


FaultID FaultSet3D::getFaultID( int idx ) const
{
    return ids_.validIdx(idx) ? ids_[idx] : FaultID::udf();
}


FaultID FaultSet3D::addFault( RefMan<Fault3D> flt )
{
    flt->ref();
    faults_.add( flt.ptr() );
    FaultID newid( ++curidnr_ );
    ids_.add( newid );
    return newid;
}


bool FaultSet3D::addFault( RefMan<Fault3D> flt, FaultID fid )
{
    if ( ids_.isPresent(fid) )
	return false;

    flt->ref();
    faults_.add( flt.ptr() );
    ids_.add( fid );
    return true;
}


bool FaultSet3D::removeFault( FaultID fid )
{
    const int idx = indexOf( fid );
    if ( !ids_.validIdx(idx) )
	return false;

    ids_.removeSingle( idx );
    faults_.removeSingle( idx )->unRef();
    return true;
}


RefMan<Fault3D> FaultSet3D::getFault3D( FaultID fid )
{
    const int idx = indexOf( fid );
    return ids_.validIdx(idx) ? faults_[idx] : nullptr;
}


ConstRefMan<Fault3D> FaultSet3D::getFault3D( FaultID fid ) const
{
    const int idx = indexOf( fid );
    return ids_.validIdx(idx) ? faults_[idx] : nullptr;
}


int FaultSet3D::indexOf( FaultID fid ) const
{
    return ids_.indexOf( fid );
}


Executor* FaultSet3D::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    PtrMan<EMFaultSet3DTranslator> transl =
		sCast(EMFaultSet3DTranslator*,ioobj->createTranslator());
    if ( !transl )
	return nullptr;

    return transl->writer( *this, *ioobj );
}

} // namespace EM
