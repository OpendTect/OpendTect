/*+
________________________________________________________________________________

 Copyright:  (C) 2022 dGB Beheer B.V.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/
#include "arrayndimpl.h"
#include "coord.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "faultstickset.h"
#include "ioman.h"
#include "ioobj.h"
#include "odjson.h"
#include "survinfo.h"
#include "task.h"
#include "zdomain.h"

#include <string.h>

#include "odsurvey.h"
#include "odfault.h"

// #ifdef __win__
//     template class __declspec(dllimport) ValueSeries<float>;
// #endif

const char* odFault3D::translatorGrp()
{
    return EMFault3DTranslatorGroup::sGroupName();
}


const char* odFaultStickSet::translatorGrp()
{
    return EMFaultStickSetTranslatorGroup::sGroupName();
}


odFaultObject::odFaultObject( const odSurvey& thesurvey, const char* name,
			const char* tgname )
    : odSurveyObject(thesurvey, name, tgname)
{}


odFaultObject::odFaultObject( const odSurvey& thesurvey, const char* name,
			const char* tgname, bool overwrite )
    : odSurveyObject(thesurvey, name, tgname, overwrite)
{}


odFaultObject::~odFaultObject()
{}


void odFaultObject::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
{
    jsobj.set( "type", "Feature" );
    auto* info = new OD::JSON::Object;
    getInfo( *info );
    jsobj.set( "properties", info );
    auto* geom = new OD::JSON::Object;
    geom->set( "type", "MultiLineString" );
    auto* coords = new OD::JSON::Array( false );
    getPoints( *coords, towgs );
    geom->set( "coordinates", coords );
    jsobj.set( "geometry", geom );
}


void odFaultObject::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    const auto* fss = stickset();
    if ( !fss )
    {
	errmsg_ = "Bad stick set";
	return;
    }

    for ( int idx=0; idx<fss->nrSticks(); idx++ )
    {
	TypeSet<Coord> coords;
	const TypeSet<Coord3>* stick = fss->getStick( idx );
	for ( const auto& node : *stick )
	    coords += node.coord();

	auto* jscoords = new OD::JSON::Array( false );
	survey_.makeCoordsList( *jscoords, coords, towgs );
	jsarr.add( jscoords );
    }
}


int odFaultObject::getNrSticks() const
{
    survey_.activate();
    const auto* fss = stickset();
    if ( !fss )
    {
	errmsg_ = "Bad stick set";
	return -1;
    }

    return fss->nrSticks();
}


void odFaultObject::getStick( int idx, hAllocator allocator ) const
{
    survey_.activate();
    const auto* fss = stickset();
    if ( !fss )
    {
	errmsg_ = "Bad stick set";
	return;
    }

    if ( idx<0 || idx>=fss->nrSticks() )
    {
	errmsg_ = "Bad stick index";
	return;
    }

    const int ndim_xy = 1;
    PtrMan<int> dims_xy = new int[ndim_xy];
    const TypeSet<Coord3>* stick = fss->getStick( idx );
    dims_xy[0] = stick->size();
    double* xdata = static_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    double* ydata = static_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    double* zdata = static_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    for ( const auto& node : *stick )
    {
	*xdata++ = node.x;
	*ydata++ = node.y;
	*zdata++ = node.z;
    }
}


Geometry::FaultStickSet* odFaultObject::stickset() const
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return nullptr;

    EM::EMM().loadIfNotFullyLoaded( ioobj->key() );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( ioobj->key() );
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement())
    return fss;
}


odFault3D::odFault3D( const odSurvey& thesurvey, const char* name )
    : odFaultObject(thesurvey, name, translatorGrp())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "Error opening Fault3D: ";

	errmsg_.add( "invalid eminfo." );
    }
}


odFault3D::odFault3D( const odSurvey& thesurvey, const char* name,
			  const StepInterval<int>& inl_rg,
			  const StepInterval<int>& crl_rg, bool overwrite )
    : odFaultObject(thesurvey, name, translatorGrp(), overwrite)
{
}


odFault3D::~odFault3D()
{
}


void odFault3D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    jsobj.set( "name", getName().buf() );
    if ( ioobj )
    {
	const EM::IOObjInfo eminfo( ioobj.ptr() );
	jsobj.set( "stick_count", eminfo.nrSticks() );
	jsobj.set( "inl_range", eminfo.getInlRange() );
	jsobj.set( "crl_range", eminfo.getCrlRange() );
	Interval<float> zrg = eminfo.getZRange();
	if ( zrg.isUdf() )
	    jsobj.set( "z_range", "[]" );
	else
	{
	    zrg.start *= SI().showZ2UserFactor();
	    zrg.stop *= SI().showZ2UserFactor();
	    jsobj.set( "z_range", zrg );
	}
    }
}


odFaultStickSet::odFaultStickSet( const odSurvey& thesurvey, const char* name )
    : odFaultObject(thesurvey, name, translatorGrp())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "Error opening fault stickset: ";

	errmsg_.add( "invalid eminfo." );
    }
}


odFaultStickSet::odFaultStickSet( const odSurvey& thesurvey, const char* name,
				  bool overwrite )
    : odFaultObject(thesurvey, name, translatorGrp(), overwrite)
{

}


odFaultStickSet::~odFaultStickSet()
{
}


void odFaultStickSet::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    jsobj.set( "name", getName().buf() );
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( ioobj )
    {
	const EM::IOObjInfo eminfo( ioobj.ptr() );
	jsobj.set( "stick_count", eminfo.nrSticks() );
	Interval<float> zrg = eminfo.getZRange();
	if ( zrg.isUdf() )
	    jsobj.set( "z_range", "[]" );
	else
	{
	    zrg.start *= SI().showZ2UserFactor();
	    zrg.stop *= SI().showZ2UserFactor();
	    jsobj.set( "z_range", zrg );
	}
    }
}


// Fault3D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Fault3D, fault3d)
mDefineRemoveBindings(Fault3D, fault3d)

void fault3d_getstick( hFault3D self, int idx, hAllocator allocator )
{
    const auto* p = static_cast<odFault3D*>(self);
    if	( !p || !p->canRead() )
	return;

    p->getStick( idx, allocator );
}


int fault3d_stickcount( hFault3D self )
{
    const auto* p = static_cast<odFault3D*>(self);
    if	( !p || !p->canRead() )
	return -1;

    return p->getNrSticks();
}


// FaultStickSet bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(FaultStickSet, faultstickset)
mDefineRemoveBindings(FaultStickSet, faultstickset)

void faultstickset_getstick( hFaultStickSet self, int idx,
			     hAllocator allocator )
{
    const auto* p = static_cast<odFaultStickSet*>(self);
    if	( !p || !p->canRead() )
	return;

    p->getStick( idx, allocator );
}


int faultstickset_stickcount( hFaultStickSet self )
{
    const auto* p = static_cast<odFaultStickSet*>(self);
    if	( !p || !p->canRead() )
	return -1;

    return p->getNrSticks();
}
