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

const char* odHorizon3D::sKeyTranslatorGrp()
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
    survey_.activate();

    const EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return names;

    names = new BufferStringSet;
    eminfo.getAttribNames( *names );
    return names;
}


int odEMObject::getNrAttributes() const
{
    const EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return 0;

    BufferStringSet tmp;
    eminfo.getAttribNames( tmp );
    return tmp.size();
}


odHorizon3D::odHorizon3D( const odSurvey& thesurvey, const char* name )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp())
{
    if ( !ioobj_ )
	return;

    const EM::IOObjInfo eminfo( ioobj_ );
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
    : odEMObject(thesurvey, name, sKeyTranslatorGrp(), overwrite)
{
    tk_.set( inl_rg, crl_rg );
    array_ = new Array2DImpl<float>( tk_.nrInl(), tk_.nrCrl() );
    array_->setAll( mUdf(float) );
}


odHorizon3D::~odHorizon3D()
{
}


void odHorizon3D::save()
{
    survey_.activate();
    if ( ioobj_ && array_ && writecount_ )
    {
	const EM::IOObjInfo eminfo( ioobj_ );
	RefMan<EM::Horizon3D> hor3d;
	if ( eminfo.isOK() && eminfo.isHorizon() )
	{
	    const MultiID hor3dkey = ioobj_->key();
	    RefMan<EM::EMObject> obj = EM::EMM().loadIfNotFullyLoaded(hor3dkey);
	    if ( !obj )
	    {
		errmsg_ = "Horizon3D::save - invalid emobject.";
		return;
	    }

	    hor3d = reinterpret_cast<EM::Horizon3D*>( obj.ptr() );
	}
	else
	    hor3d = EM::Horizon3D::create( name_ );

	if ( !hor3d )
	{
	    errmsg_ = "Horizon3D::save - invalid object.";
	    return;
	}

	hor3d->setMultiID( ioobj_->key() );
	if ( hor3d->setArray2D(array_, tk_.start_, tk_.step_) )
	{
	    PtrMan<Executor> saver = hor3d->saver();
	    if (!saver || !TaskRunner::execute(nullptr, *saver.ptr()) )
	    {
		errmsg_ = "Horizon3D::save - error during save.";
		return;
	    }
	}
    }

    writecount_ = 0;
}


void odHorizon3D::getInfo( OD::JSON::Object& jsobj ) const
{
    survey_.activate();
    jsobj.setEmpty();
    jsobj.set( "name", getName().buf() );
    jsobj.set( "inl_range", tk_.lineRange() );
    jsobj.set( "crl_range", tk_.trcRange() );
    if ( ioobj_ )
    {
	const EM::IOObjInfo eminfo( ioobj_ );
	Interval<float> zrg = eminfo.getZRange();
	zrg.start *= SI().showZ2UserFactor();
	zrg.stop *= SI().showZ2UserFactor();
	jsobj.set( "z_range", zrg );
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
    survey_.activate();
    if ( !ioobj_ )
    {
	errmsg_ = "Horizon3D::get - invalid ioobj.";
	return;
    }

    if ( !array_ )
    {
	const MultiID hor3dkey = ioobj_->key();
	RefMan<EM::EMObject> obj = EM::EMM().loadIfNotFullyLoaded(hor3dkey);
	if ( !obj )
	{
	    errmsg_ = "Horizon3D::get - invalid emobject.";
	    return;
	}

	mDynamicCastGet(EM::Horizon3D*,hor,obj.ptr());
	if ( !hor )
	{
	    errmsg_ = "Horizon3D::get - invalid object.";
	    return;
	}

	array_ = hor->createArray2D();
    }

    if ( !array_ || !isOK() )
	return;

    const int ndim = 2;
    int dims[ndim];
    for ( int i=0; i<ndim; i++ )
	dims[i] = array_->info().getSize(i);

    float* data = reinterpret_cast<float*>( allocator(ndim, dims, 'f') );
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
	errmsg_ = "Horizon3D::getXY - invalid horizon geometry";
	return;
    }

    const int ndim = 2;
    int dims[ndim];
    dims[0] = tk_.nrLines();
    dims[1] = tk_.nrTrcs();
    double* xdata = reinterpret_cast<double*>( allocator(ndim, dims, 'd') );
    double* ydata = reinterpret_cast<double*>( allocator(ndim, dims, 'd') );
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


void odHorizon3D::putZ( const uint32_t shape[2], const float* data,
			const int32_t* inlines, const int32_t* crlines)
{
    TrcKey trckey;
    trckey.setIs2D( false );
    const float zfac = SI().showZ2UserFactor();
    writecount_ = 0;
    for ( int xdx=0; xdx<shape[0]; xdx++ )
    {
	const int32_t inl = inlines[xdx];
	for ( int ydx=0; ydx<shape[1]; ydx++ )
	{
	    const int32_t crl = crlines[ydx];
	    trckey.setLineNr( inl );
	    trckey.setTrcNr( crl );
	    if ( tk_.includes(trckey) )
	    {
		float val = data[xdx*shape[1]+ydx];
#ifdef __win__
		if ( !isnan(val) )
#else
		if ( !std::isnan(val) )
#endif
		{
		    val /= zfac;
		    array_->set( tk_.lineIdx(trckey.inl()),
				 tk_.trcIdx(trckey.crl()), val );
		    writecount_++;
		}
	    }
	}
    }

    save();
}


void odHorizon3D::putZ( const uint32_t shape[2], const float* data,
			const double* xpos, const double* ypos)
{
    TrcKey trckey;
    trckey.setIs2D( false );
    const float zfac = SI().showZ2UserFactor();
    writecount_ = 0;
    for ( int xdx=0; xdx<shape[0]; xdx++ )
    {
	for ( int ydx=0; ydx<shape[1]; ydx++ )
	{
	    const int idx = xdx*shape[1]+ydx;
	    const Coord pos( xpos[idx], ypos[idx] );
	    trckey.setFrom( pos );
	    if ( tk_.includes(trckey) )
	    {
		float val = data[idx];
#ifdef __win__
		if ( !isnan(val) )
#else
		if ( !std::isnan(val) )
#endif
		{
		    val /= zfac;
		    array_->set( tk_.lineIdx(trckey.inl()),
				 tk_.trcIdx(trckey.crl()), val );
		    writecount_++;
		}
	    }
	}
    }

    save();
}


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp())
{
    if ( !ioobj_ )
	return;

    const EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
    {
	if ( errmsg_.isEmpty() )
	    errmsg_ = "Error opening horizon: ";

	errmsg_.add( "invalid eminfo." );
    }

    const MultiID hor2dkey = ioobj_->key();
    EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded(hor2dkey);
    if ( !obj )
    {
	errmsg_ = "Horizon2D - invalid emobject.";
	return;
    }

    mDynamicCastGet(EM::Horizon2D*, hor, obj );
    if ( !hor )
    {
	errmsg_ = "Horizon2D - invalid object.";
	return;
    }
    hor_ = hor;
}


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name,
			  bool creategeom, bool overwrite )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp(), overwrite)
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
    survey_.activate();
    jsobj.setEmpty();
    const EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return;

    jsobj.set( "name", getName().buf() );
    jsobj.set( "z_range", eminfo.getZRange() );
    jsobj.set( "line_count", getNrLines() );
    jsobj.set( "attrib_count", getNrAttributes() );
}


BufferStringSet* odHorizon2D::getLineNames() const
{
    survey_.activate();
    BufferStringSet* names = nullptr;
    const EM::IOObjInfo eminfo( ioobj_ );
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
    survey_.activate();
    TypeSet<Pos::GeomID> geomids;
    const EM::IOObjInfo eminfo( ioobj_ );
    if ( eminfo.isOK() )
	eminfo.getGeomIDs( geomids );

    return geomids.size();
}


void odHorizon2D::getLineIDs( int num, int* ids ) const
{
    survey_.activate();
    const EM::IOObjInfo eminfo( ioobj_ );
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
	errmsg_ = "Horizon2D::getZ - invalid object.";
	return;
    }

    survey_.activate();
    Pos::GeomID geomid( lineid );
    PtrMan<Array1D<float>> array = hor_->createArray1D( geomid );
    if ( !array )
    {
	errmsg_ = "Horizon2D::getZ - lineid not found.";
	return;
    }

    const int ndim = 1;
    int dims[ndim];
    for ( int i=0; i<ndim; i++ )
	dims[i] = array->info().getSize(i);

    float* data = reinterpret_cast<float*>( allocator(ndim, dims, 'f') );
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
	errmsg_ = "Horizon2D::getZ - invalid object.";
	return;
    }

    survey_.activate();
    Pos::GeomID geomid( lineid );
    if ( !hor_->geometry().hasLine(geomid) )
    {
	errmsg_ = "Horizon2D::getXY - lineid not found.";
	return;
    }

    const StepInterval<int> trcrg = hor_->geometry().colRange( geomid );
    const int ntrc = trcrg.nrSteps()+1;
    const int ndim = 1;
    int dims[ndim];
    dims[0] = ntrc;
    double* xdata = reinterpret_cast<double*>( allocator(ndim, dims, 'd') );
    double* ydata = reinterpret_cast<double*>( allocator(ndim, dims, 'd') );
    int32_t* trc = reinterpret_cast<int32_t*>( allocator(ndim, dims, 'i') );
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
hHorizon3D horizon3d_newout( hSurvey survey, const char* name,
			     const int* inl_rg, const int* crl_rg,
			     bool overwrite )
{
    const auto* p = reinterpret_cast<odSurvey*>(survey);
    if ( !p ) return nullptr;
    return new odHorizon3D( *p, name,
			    StepInterval<int>(inl_rg[0], inl_rg[1], inl_rg[2]),
			    StepInterval<int>(crl_rg[0], crl_rg[1], crl_rg[2]),
			    overwrite  );
}


hStringSet horizon3d_attribnames( hHorizon3D self )
{
    const auto* p = reinterpret_cast<odHorizon3D*>(self);
    if ( !p ) return nullptr; \
    return p->getAttribNames();
}


void horizon3d_getz( hHorizon3D self, hAllocator allocator )
{
    auto* p = reinterpret_cast<odHorizon3D*>(self);
    if ( p )
	p->getZ( allocator );
}


void horizon3d_getxy( hHorizon3D self , hAllocator allocator )
{
    auto* p = reinterpret_cast<odHorizon3D*>(self);
    if ( p )
	p->getXY( allocator );
}


void horizon3d_putz( hHorizon3D self, const uint32_t shape[2],
		     const float* data, const int32_t* inlines,
		     const int32_t* crlines )
{
    auto* p = reinterpret_cast<odHorizon3D*>(self);
    if ( p )
	p->putZ( shape, data, inlines, crlines );
}


void horizon3d_putz_byxy( hHorizon3D self, const uint32_t shape[2],
			  const float* data,
			  const double* xpos, const double* ypos )
{
    auto* p = reinterpret_cast<odHorizon3D*>(self);
    if ( p )
	p->putZ( shape, data, xpos, ypos );
}


// Horizon2D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Horizon2D, horizon2d)
hHorizon2D horizon2d_newout( hSurvey survey, const char* name,
			     bool creategeom, bool overwrite )
{
    const auto* p = reinterpret_cast<odSurvey*>(survey);
    if ( !p ) return nullptr;
    return new odHorizon2D( *p, name, creategeom, overwrite  );
}


hStringSet horizon2d_attribnames( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    if ( !p ) return nullptr;
    return p->getAttribNames();
}


int horizon2d_linecount( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p ? p->getNrLines() : 0;
}


void horizon2d_lineids( hHorizon2D self, int num, int* ids )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    if ( p )
	p->getLineIDs( num, ids );
}

const char* horizon2d_linename( hHorizon2D self, int  lineid )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p ? strdup( p->getLineName(lineid).buf() ) : nullptr;
}

hStringSet horizon2d_linenames( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p ? p->getLineNames() : nullptr;
}


void horizon2d_getz( hHorizon2D self, hAllocator allocator, int lineid )
{
    auto* p = reinterpret_cast<odHorizon2D*>(self);
    if ( p )
	p->getZ( allocator, lineid );
}


void horizon2d_getxy( hHorizon2D self , hAllocator allocator, int lineid )
{
    auto* p = reinterpret_cast<odHorizon2D*>(self);
    if ( p )
	p->getXY( allocator, lineid );
}



