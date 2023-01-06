/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Sep 2018
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
    , curidnr_(0)
{
    setPreferredColor( getRandomColor() );
    setPreferredMarkerStyle3D(
	MarkerStyle3D(MarkerStyle3D::Cube,3,Color::Yellow()) );
}


FaultSet3D::~FaultSet3D()
{
    deepUnRef( faults_ );
}


Executor* FaultSet3D::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return 0;

    PtrMan<EMFaultSet3DTranslator> transl =
                        (EMFaultSet3DTranslator*)ioobj->createTranslator();
    return transl ? transl->reader( *this, *ioobj ) : 0;
}


const IOObjContext& FaultSet3D::getIOObjContext() const
{ return EMFaultSet3DTranslatorGroup::ioContext(); }

uiString FaultSet3D::getUserTypeStr() const
{ return EMFaultSet3DTranslatorGroup::sTypeName(); }

int FaultSet3D::nrFaults() const
{ return faults_.size(); }

FaultID FaultSet3D::getFaultID( int idx ) const
{ return ids_.validIdx(idx) ? ids_[idx] : -1; }

FaultID FaultSet3D::addFault( RefMan<Fault3D> flt )
{
    flt->ref();
    faults_.add( flt.ptr() );
    FaultID newid = ++curidnr_;
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
    return ids_.validIdx(idx) ? faults_[idx] : 0;
}


ConstRefMan<Fault3D> FaultSet3D::getFault3D( FaultID fid ) const
{
    const int idx = indexOf( fid );
    return ids_.validIdx(idx) ? faults_[idx] : 0;
}


int FaultSet3D::indexOf( FaultID fid ) const
{ return ids_.indexOf( fid ); }


Executor* FaultSet3D::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    PtrMan<EMFaultSet3DTranslator> transl =
	(EMFaultSet3DTranslator*)ioobj->createTranslator();
    if ( !transl )
	return 0;

    return transl->writer( *this, *ioobj );
}

} // namespace EM
