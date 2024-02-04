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
#include "emhorizonutils.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "odjson.h"
#include "survinfo.h"
#include "task.h"
#include "zdomain.h"

#include <string.h>

#include "odsurvey.h"
#include "odhorizon.h"

// #ifdef __win__
//     template class __declspec(dllimport) ValueSeries<float>;
// #endif

const char* odHorizon3D::translatorGrp()
{
    return EMHorizon3DTranslatorGroup::sGroupName();
}


odEMObject::odEMObject( const odSurvey& thesurvey, const char* name,
			const char* tgname )
    : odSurveyObject(thesurvey, name, tgname)
{}


odEMObject::odEMObject( const odSurvey& thesurvey, const char* name,
			const char* tgname, bool overwrite )
    : odSurveyObject(thesurvey, name, tgname, overwrite)
{}


odEMObject::~odEMObject()
{}


BufferStringSet* odEMObject::getAttribNames() const
{
    BufferStringSet* names = nullptr;
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return names;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
	return names;

    names = new BufferStringSet;
    eminfo.getAttribNames( *names );
    return names;
}


int odEMObject::getNrAttributes() const
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return 0;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
	return 0;

    BufferStringSet tmp;
    eminfo.getAttribNames( tmp );
    return tmp.size();
}


odHorizon3D::odHorizon3D( const odSurvey& thesurvey, const char* name )
    : odEMObject(thesurvey, name, translatorGrp())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( eminfo.isOK() )
	tk_.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    else
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "Error opening horizon: ";

	errmsg_.add( "invalid eminfo." );
    }
}


odHorizon3D::odHorizon3D( const odSurvey& thesurvey, const char* name,
			  const StepInterval<int>& inl_rg,
			  const StepInterval<int>& crl_rg, bool overwrite )
    : odEMObject(thesurvey, name, translatorGrp(), overwrite)
{
    tk_.set( inl_rg, crl_rg );
    array_ = new Array2DImpl<float>( tk_.nrInl(), tk_.nrCrl() );
    array_->setAll( mUdf(float) );
}


odHorizon3D::~odHorizon3D()
{
}


RefMan<EM::Horizon3D> odHorizon3D::getHorizonObj( bool create)
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return nullptr;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    RefMan<EM::Horizon3D> hor3d;
    if ( eminfo.isOK() && EM::isHorizon(eminfo.type()) )
    {
	const MultiID hor3dkey = ioobj->key();
	EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded(hor3dkey);
	if ( !obj )
	{
	    errmsg_ = "odHorizon3D::getHorizonObj - invalid emobject.";
	    return nullptr;
	}

	hor3d = static_cast<EM::Horizon3D*>( obj );
    }
    else if ( create )
    {
	hor3d = EM::Horizon3D::create( name_);
	if ( hor3d )
	    hor3d->setMultiID( ioobj->key() );
    }

    if ( !hor3d )
	errmsg_ = "odHorizon3D::getHorizonObj - invalid horizon object.";

    return hor3d;
}


void odHorizon3D::save()
{
    if ( array_ && writecount_ )
    {
	RefMan<EM::Horizon3D> hor3d = getHorizonObj( true );
	ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
	if ( !hor3d )
	    return;

	if ( hor3d->setArray2D(array_, tk_.start_, tk_.step_) )
	{
	    PtrMan<Executor> saver = hor3d->saver();
	    if (!saver || !TaskRunner::execute(nullptr, *saver.ptr()) )
	    {
		errmsg_ = "odHorizon3D::save - error during save.";
		return;
	    }
	}
    }

    writecount_ = 0;
}


void odHorizon3D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    jsobj.set( "name", getName().buf() );
    jsobj.set( "inl_range", tk_.lineRange() );
    jsobj.set( "crl_range", tk_.trcRange() );
    if ( ioobj )
    {
	const EM::IOObjInfo eminfo( ioobj.ptr() );
	Interval<float> zrg = eminfo.getZRange();
	zrg.start *= SI().showZ2UserFactor();
	zrg.stop *= SI().showZ2UserFactor();
	jsobj.set( "z_range", zrg );
	jsobj.set( "zunit", eminfo.getZUnitLabel() );
    }
    jsobj.set( "attrib_count", getNrAttributes() );

}


void odHorizon3D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    TypeSet<Coord> coords;
    for ( int i=0; i<4; i++ )
	coords += tk_.toCoord( tk_.corner(i) );

    coords.swap( 2, 3 );
    coords += coords[0];
    survey_.makeCoordsList( jsarr, coords, towgs );
}


void odHorizon3D::getZ( hAllocator allocator )
{
    if ( !array_ )
    {
	RefMan<EM::Horizon3D> hor = getHorizonObj();
	if ( !hor )
	    return;

	array_ = hor->createArray2D();
    }

    if ( !array_ || !isOK() )
	return;

    const int ndim = 2;
    int dims[ndim];
    for ( int i=0; i<ndim; i++ )
	dims[i] = array_->info().getSize(i);

    float* data = static_cast<float*>( allocator(ndim, dims, 'f') );
    const float zfac = SI().showZ2UserFactor();
    const float znan = std::nanf("");
    for (int i=0; i<dims[0]; i++)
    {
	for (int j=0; j<dims[1]; j++)
	{
	    float z = array_->get( i, j );
	    if ( mIsUdf(z) )
		z = znan;
	    else
		z *= zfac;

	    *data++ = z;
	}
    }
}


void odHorizon3D::getXY( hAllocator allocator )
{
    survey_.activate();
    errmsg_.setEmpty();
    if ( tk_.isEmpty() )
    {
	errmsg_ = "odHorizon3D::getXY - invalid horizon geometry";
	return;
    }

    const int ndim = 2;
    int dims[ndim];
    dims[0] = tk_.nrLines();
    dims[1] = tk_.nrTrcs();
    double* xdata = static_cast<double*>( allocator(ndim, dims, 'd') );
    double* ydata = static_cast<double*>( allocator(ndim, dims, 'd') );
    for (int xdx=0; xdx<dims[0]; xdx++)
    {
	const int line = tk_.lineID( xdx );
	for (int ydx=0; ydx<dims[1]; ydx++)
	{
	    const int trc = tk_.traceID( ydx );
	    const BinID bid( line, trc );
	    const Coord pos = tk_.toCoord( bid );
	    *xdata++ = pos.x;
	    *ydata++ = pos.y;
	}
    }
}


void odHorizon3D::getAuxData( hAllocator allocator, const char* auxname )
{
    RefMan<EM::Horizon3D> hor = getHorizonObj();
    if ( !hor )
	return;

    PtrMan<Executor> auxloader = hor->auxdata.auxDataLoader( auxname );
    if ( !auxloader || !TaskRunner::execute( nullptr, *auxloader ) )
    {
	errmsg_ = "odHorizon3D::getAuxData - error loading attribute.";
	return;
    }

    if ( hor->auxdata.hasAuxDataName(auxname) )
    {
	int iaux = hor->auxdata.auxDataIndex( auxname );
	PtrMan<Array2D<float>> array = hor->auxdata.createArray2D( iaux );
	const int ndim = 2;
	int dims[ndim];
	for ( int i=0; i<ndim; i++ )
	    dims[i] = array->info().getSize(i);

	float* data = static_cast<float*>( allocator(ndim, dims, 'f') );
	const float valnan = std::nanf("");
	for (int i=0; i<dims[0]; i++)
	{
	    for (int j=0; j<dims[1]; j++)
	    {
		float val = array->get( i, j );
		if ( mIsUdf(val) )
		    val = valnan;

		*data++ = val;
	    }
	}
    }
}


void odHorizon3D::putZ( const float* data, const TrcKeySampling& tk )
{
    errmsg_.setEmpty();
    if ( !canWrite() )
	return;

    TrcKey trckey;
    trckey.setIs2D( false );
    const float zfac = SI().showZ2UserFactor();
    writecount_ = 0;
    for ( od_int64 idx=0; idx<tk.totalNr(); idx++ )
    {
	trckey = tk.trcKeyAt( idx );
	if ( !tk_.includes(trckey, false) )
	    continue;

	float val = data[idx];
	if ( !Math::IsNormalNumber(val) )
	    val = mUdf(float);
	else
	    val /= zfac;

	array_->set( tk_.lineIdx(trckey.inl()), tk_.trcIdx(trckey.crl()), val );
	writecount_++;
    }

    save();
}


void odHorizon3D::putAuxData( const char* name, const float* data,
			      const TrcKeySampling& tk )
{
    errmsg_.setEmpty();
    if ( !canWrite() )
	return;

    RefMan<EM::Horizon3D> hor3d = getHorizonObj( true );
    if ( !hor3d )
	return;

    int auxidx = hor3d->auxdata.auxDataIndex( name );
    if ( auxidx==-1 )
	auxidx = hor3d->auxdata.addAuxData( name );
    for ( od_int64 idx=0; idx<tk.totalNr(); idx++ )
    {
	const TrcKey trckey = tk.trcKeyAt( idx );
	if ( !tk_.includes(trckey, false) )
	    continue;

	float val = data[idx];
	if ( !Math::IsNormalNumber(val) )
	    val = mUdf(float);

	hor3d->auxdata.setAuxDataVal( auxidx, trckey, val );
    }

    PtrMan<Executor> auxsaver = hor3d->auxdata.auxDataSaver( auxidx, true );
    if ( !auxsaver || !TaskRunner::execute( nullptr, *auxsaver ) )
	errmsg_ = BufferString(
		"odHorizon3D::putAuxData - error saving attribute: ", name );

}


// void odHorizon3D::putZ( const uint32_t shape[2], const float* data,
//			const double* xpos, const double* ypos)
// {
//     TrcKey trckey;
//     trckey.setIs2D( false );
//     const float zfac = SI().showZ2UserFactor();
//     writecount_ = 0;
//     for ( int xdx=0; xdx<shape[0]; xdx++ )
//     {
//	for ( int ydx=0; ydx<shape[1]; ydx++ )
//	{
//	    const int idx = xdx*shape[1]+ydx;
//	    const Coord pos( xpos[idx], ypos[idx] );
//	    trckey.setFrom( pos );
//	    if ( tk_.includes(trckey) )
//	    {
//		float val = data[idx];
// #ifdef __win__
//		if ( !isnan(val) )
// #else
//		if ( !std::isnan(val) )
// #endif
//		{
//		    val /= zfac;
//		    array_->set( tk_.lineIdx(trckey.inl()),
//				 tk_.trcIdx(trckey.crl()), val );
//		    writecount_++;
//		}
//	    }
//	}
//     }
//
//     save();
// }


bool odHorizon3D::deleteAttribs( const BufferStringSet& attribnms )
{
    RefMan<EM::Horizon3D> hor = getHorizonObj();
    if ( !hor )
	return false;

    for ( auto* attrib : attribnms )
    {
	int idx = hor->auxdata.auxDataIndex( *attrib );
	if ( idx>0 )
	    hor->auxdata.removeAuxData( idx );

	hor->auxdata.removeFile( *attrib );
    }
    return true;
}


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name )
    : odEMObject(thesurvey, name, translatorGrp())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "Error opening horizon: ";

	errmsg_.add( "invalid eminfo." );
    }

    const MultiID hor2dkey = ioobj->key();
    EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded(hor2dkey);
    if ( !obj )
    {
	errmsg_ = "odHorizon2D - invalid emobject.";
	return;
    }

    mDynamicCastGet(EM::Horizon2D*, hor, obj );
    if ( !hor )
    {
	errmsg_ = "odHorizon2D - invalid object.";
	return;
    }
    hor_ = hor;
}


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name,
			  bool creategeom, bool overwrite )
    : odEMObject(thesurvey, name, translatorGrp(), overwrite)
    , creategeom_(creategeom)
{

}


odHorizon2D::~odHorizon2D()
{
}


// void odHorizon2D::close()
// {
//     survey_.activate();
//     if ( array_ && writecount_ )
//     {
// 	RefMan<EM::Horizon3D> hor3d = EM::Horizon3D::create( name_ );
// 	if ( !hor3d )
// 	    throw( pybind11::value_error("cannot create horizon") );
//
// 	hor3d->setMultiID( ioobj_->key() );
// 	if ( hor3d->setArray2D(array_, tk_.start_, tk_.step_) )
// 	{
// 	    PtrMan<Executor> saver = hor3d->saver();
// 	    if (!saver || !TaskRunner::execute(nullptr, *saver.ptr()) )
// 		throw( pybind11::value_error("failed during horizon save") );
// 	}
//     }
//
//     if ( writecount_==0 )
// 	IOM().implRemove( *ioobj_ );
//
//     delete array_;
//     array_ = nullptr;
//     writecount_ = 0;
// }


void odHorizon2D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
	return;

    jsobj.set( "name", getName().buf() );
    jsobj.set( "z_range", eminfo.getZRange() );
    jsobj.set( "line_count", getNrLines() );
    jsobj.set( "attrib_count", getNrAttributes() );
}


BufferStringSet* odHorizon2D::getLineNames() const
{
    BufferStringSet* names = nullptr;
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return names;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( !eminfo.isOK() )
	return names;

    names = new BufferStringSet;
    eminfo.getLineNames( *names );
    return names;
}


BufferString odHorizon2D::getLineName( int lineid ) const
{
    const int lidx = hor_->geometry().lineIndex( Pos::GeomID(lineid) );
    return BufferString( hor_->geometry().lineName(lidx) );
}


int odHorizon2D::getNrLines() const
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return 0;

    TypeSet<Pos::GeomID> geomids;
    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( eminfo.isOK() )
	eminfo.getGeomIDs( geomids );

    return geomids.size();
}


void odHorizon2D::getLineIDs( int num, int* ids ) const
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj.ptr() );
    if ( eminfo.isOK() )
    {
	TypeSet<Pos::GeomID> geomids;
	eminfo.getGeomIDs( geomids );
	for ( int idx=0; idx<num; idx++ )
	    if( geomids.validIdx(idx) )
		ids[idx] = geomids[idx].asInt();
    }
}


void odHorizon2D::getZ( hAllocator allocator, int lineid )
{
    if ( !hor_ )
    {
	errmsg_ = "odHorizon2D::getZ - invalid object.";
	return;
    }

    survey_.activate();
    Pos::GeomID geomid( lineid );
    PtrMan<Array1D<float>> array = hor_->createArray1D( geomid );
    if ( !array )
    {
	errmsg_ = "odHorizon2D::getZ - lineid not found.";
	return;
    }

    const int ndim = 1;
    int dims[ndim];
    for ( int i=0; i<ndim; i++ )
	dims[i] = array->info().getSize(i);

    float* data = static_cast<float*>( allocator(ndim, dims, 'f') );
    const float zfac = SI().showZ2UserFactor();
    const float znan = std::nanf("");
    for (int i=0; i<dims[0]; i++)
    {
	float z = array->get( i );
	if ( mIsUdf(z) )
	    z = znan;
	else
	    z *= zfac;

	*data++ = z;
    }
}


void odHorizon2D::getXY( hAllocator allocator, int lineid )
{
    if ( !hor_ )
    {
	errmsg_ = "odHorizon2D::getZ - invalid object.";
	return;
    }

    survey_.activate();
    Pos::GeomID geomid( lineid );
    if ( !hor_->geometry().hasLine(geomid) )
    {
	errmsg_ = "odHorizon2D::getXY - lineid not found.";
	return;
    }

    const StepInterval<int> trcrg = hor_->geometry().colRange( geomid );
    const int ntrc = trcrg.nrSteps()+1;
    const int ndim = 1;
    int dims[ndim];
    dims[0] = ntrc;
    double* xdata = static_cast<double*>( allocator(ndim, dims, 'd') );
    double* ydata = static_cast<double*>( allocator(ndim, dims, 'd') );
    int32_t* trc = static_cast<int32_t*>( allocator(ndim, dims, 'i') );
    TrcKey tk( geomid, -1 );
    for (int idx=0; idx<dims[0]; idx++)
    {
	const int trcnr = trcrg.atIndex( idx );
	tk.setTrcNr( trcnr );
	const Coord pos = hor_->getCoord( tk );
	*xdata++ = pos.x;
	*ydata++ = pos.y;
	*trc++ = trcnr;
    }
}


void odHorizon2D::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
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


void odHorizon2D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    const auto* h2dline = hor_->geometry().geometryElement();
    for ( int idx=0; idx<h2dline->nrLines(); idx++ )
    {
	const Pos::GeomID geomid = h2dline->geomID( idx );
	PosInfo::Line2DData l2d;
	h2dline->geometry( geomid, l2d );
	const auto& l2dpos = l2d.positions();
	TypeSet<Coord> coords;
	coords += l2dpos.first().coord_;
	coords += l2dpos.last().coord_;
	auto* jscoords = new OD::JSON::Array( false );
	survey_.makeCoordsList( *jscoords, coords, towgs );
	jsarr.add( jscoords );
    }
}


// Horizon3D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Horizon3D, horizon3d)
mDefineRemoveBindings(Horizon3D, horizon3d)

hHorizon3D horizon3d_newout( hSurvey survey, const char* name,
			     const int* inl_rg, const int* crl_rg,
			     bool overwrite )
{
    const auto* p = static_cast<odSurvey*>(survey);
    if ( !p ) return nullptr;
    return new odHorizon3D( *p, name,
			    StepInterval<int>(inl_rg[0], inl_rg[1], inl_rg[2]),
			    StepInterval<int>(crl_rg[0], crl_rg[1], crl_rg[2]),
			    overwrite  );
}


hStringSet horizon3d_attribnames( hHorizon3D self )
{
    const auto* p = static_cast<odHorizon3D*>(self);
    if ( !p ) return nullptr; \
    return p->getAttribNames();
}


void horizon3d_getz( hHorizon3D self, hAllocator allocator )
{
    auto* p = static_cast<odHorizon3D*>(self);
    if ( p )
	p->getZ( allocator );
}


void horizon3d_getxy( hHorizon3D self , hAllocator allocator )
{
    auto* p = static_cast<odHorizon3D*>(self);
    if ( p )
	p->getXY( allocator );
}


void horizon3d_getauxdata( hHorizon3D self, hAllocator allocator,
			   const char* auxname )
{
    auto* p = static_cast<odHorizon3D*>(self);
    if ( p )
	p->getAuxData( allocator, auxname );
}


void horizon3d_putz( hHorizon3D self, const float* data, const int32_t inlrg[3],
		     const int32_t crlrg[3] )
{
    auto* p = static_cast<odHorizon3D*>(self);
    if ( !p || !p->canWrite() )
	return;

    TrcKeySampling tktosave = odSurvey::tkFromRanges( inlrg, crlrg );
    p->putZ( data, tktosave );
}


void horizon3d_putauxdata( hHorizon3D self, const char* nm, const float* data,
			   const int32_t inlrg[3], const int32_t crlrg[3] )
{
    auto* p = static_cast<odHorizon3D*>(self);
    if ( !p || !p->canWrite() )
	return;

    TrcKeySampling tktosave = odSurvey::tkFromRanges( inlrg, crlrg );
    p->putAuxData( nm, data, tktosave );
}


// void horizon3d_putz_byxy( hHorizon3D self, const uint32_t shape[2],
//			  const float* data,
//			  const double* xpos, const double* ypos )
// {
//     auto* p = static_cast<odHorizon3D*>(self);
//     if ( p )
//	p->putZ( shape, data, xpos, ypos );
// }


bool horizon3d_deleteattribs( hHorizon3D self, const hStringSet attribnms )
{
    auto* p = static_cast<odHorizon3D*>(self);
    const auto* nms = static_cast<BufferStringSet*>(attribnms);
    if ( !p || !nms ) return false;

    if ( nms->isEmpty() )
	return true;

    return p->deleteAttribs( *nms );
}


// Horizon2D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Horizon2D, horizon2d)
mDefineRemoveBindings(Horizon2D, horizon2d)

hHorizon2D horizon2d_newout( hSurvey survey, const char* name,
			     bool creategeom, bool overwrite )
{
    const auto* p = static_cast<odSurvey*>(survey);
    if ( !p ) return nullptr;
    return new odHorizon2D( *p, name, creategeom, overwrite  );
}


hStringSet horizon2d_attribnames( hHorizon2D self )
{
    const auto* p = static_cast<odHorizon2D*>(self);
    if ( !p ) return nullptr;
    return p->getAttribNames();
}


int horizon2d_linecount( hHorizon2D self )
{
    const auto* p = static_cast<odHorizon2D*>(self);
    return p ? p->getNrLines() : 0;
}


void horizon2d_lineids( hHorizon2D self, int num, int* ids )
{
    const auto* p = static_cast<odHorizon2D*>(self);
    if ( p )
	p->getLineIDs( num, ids );
}

const char* horizon2d_linename( hHorizon2D self, int  lineid )
{
    const auto* p = static_cast<odHorizon2D*>(self);
    return p ? strdup( p->getLineName(lineid).buf() ) : nullptr;
}

hStringSet horizon2d_linenames( hHorizon2D self )
{
    const auto* p = static_cast<odHorizon2D*>(self);
    return p ? p->getLineNames() : nullptr;
}


void horizon2d_getz( hHorizon2D self, hAllocator allocator, int lineid )
{
    auto* p = static_cast<odHorizon2D*>(self);
    if ( p )
	p->getZ( allocator, lineid );
}


void horizon2d_getxy( hHorizon2D self , hAllocator allocator, int lineid )
{
    auto* p = static_cast<odHorizon2D*>(self);
    if ( p )
	p->getXY( allocator, lineid );
}



