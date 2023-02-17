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
#include "odsurvey_object.h"
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

    EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return names;

    names = new BufferStringSet;
    eminfo.getAttribNames( *names );
    return names;
}


int odEMObject::getNrAttributes() const
{
    EM::IOObjInfo eminfo( ioobj_ );
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

    EM::IOObjInfo eminfo( ioobj_ );
    if ( eminfo.isOK() )
	tk_.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    else
    {
	if ( !errmsg_.isEmpty() )
	    errmsg_.addNewLine();

	errmsg_.add( "Invalid eminfo." );
    }
}


odHorizon3D::odHorizon3D( const odSurvey& thesurvey, const char* name,
			  const StepInterval<int>& ilines,
			  const StepInterval<int>& xlines, bool overwrite )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp(), overwrite)
{
    tk_.set( ilines, xlines );
    array_ = new Array2DImpl<float>( tk_.nrInl(), tk_.nrCrl() );
    array_->setAll( mUdf(float) );
}


odHorizon3D::~odHorizon3D()
{
    if ( !forread_ )
	close();
}


void odHorizon3D::close()
{
    survey_.activate();
    errmsg_.setEmpty();
    if ( array_ && writecount_ )
    {
	RefMan<EM::Horizon3D> hor3d = EM::Horizon3D::create( name_ );
	if ( !hor3d )
	{
	    errmsg_.add( "cannot create horizon" );
	    return;
	}

	hor3d->setMultiID( ioobj_->key() );
	if ( hor3d->setArray2D(array_, tk_.start_, tk_.step_) )
	{
	    PtrMan<Executor> saver = hor3d->saver();
	    if (!saver || !TaskRunner::execute(nullptr, *saver.ptr()) )
	    {
		errmsg_.add( "failed during horizon save" );
		return;
	    }
	}
    }

    if ( writecount_==0 )
	IOM().implRemove( *ioobj_ );

    writecount_ = 0;
}


StepInterval<int> odHorizon3D::ilines() const
{
    return tk_.lineRange();
}


StepInterval<int> odHorizon3D::xlines() const
{
    return tk_.trcRange();
}


void odHorizon3D::getInfo( OD::JSON::Object& jsobj ) const
{
    survey_.activate();
    jsobj.setEmpty();
    EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return;

    jsobj.set( "Name", getName() );
    jsobj.set( "NrAttribute", getNrAttributes() );
    jsobj.set( "Z Range", eminfo.getZRange() );
    jsobj.set( "Inl Range", eminfo.getInlRange() );
    jsobj.set( "Crl Range", eminfo.getCrlRange() );
}


void odHorizon3D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    TypeSet<Coord> coords;
    for ( int i=0; i<4; i++ )
	coords += tk_.toCoord( tk_.corner(i) );

    coords.swap( 2, 3 );
    coords += coords[0];
    return survey_.makeCoordsList( jsarr, coords, towgs );
}


void odHorizon3D::getData()
{
    survey_.activate();
    errmsg_.setEmpty();
    if ( !forread_ )
    {
	errmsg_.add( "horizon only opened for writing." );
	return;
    }

    if ( !ioobj_ )
    {
	errmsg_.add( "Invalid ioobj." );
	return;
    }

    const MultiID hor3dkey = ioobj_->key();
    RefMan<EM::EMObject> obj = EM::EMM().loadIfNotFullyLoaded(hor3dkey);
    if ( !obj )
    {
	errmsg_.add( "Invalid emobject." );
	return;
    }

    mDynamicCastGet(EM::Horizon3D*,hor,obj.ptr());
    if ( !hor )
    {
	errmsg_.add( "Invalid Horizon3D." );
	return;
    }

    if ( !array_ )
	array_ = hor->createArray2D();

    // const int nx = tk_.nrLines();
    // const int ny = tk_.nrTrcs();
 //    py::array_t<float> img( {nx, ny} );
 //    auto r_img = img.mutable_unchecked<2>();
 //    const float zfac = SI().showZ2UserFactor();
 //    const float znan = std::nanf("");
 //    for ( int iln=0; iln<nx; iln++ )
 //    {
	// for ( int xln = 0; xln<ny; xln++ )
	// {
	//     float z = array->get( iln, xln);
	//     if ( mIsUdf(z) )
	// 	z = znan;
	//     else
	// 	z *= zfac;
 //
	//     r_img(iln,xln) = z;
	// }
 //    }

}

// py::object odHorizon3D::getData() const
// {
//     survey_.activate();
//     if ( !forread_ )
// 	throw( pybind11::value_error("horizon only opened for writing.") );
//
//     if ( !ioobj_ )
// 	throw( pybind11::value_error("Invalid ioobj.") );
//
//     auto XDA = py::module::import("xarray").attr("DataArray");
//
//     EM::IOObjInfo eminfo( ioobj_ );
//     if ( !eminfo.isOK() )
// 	throw( pybind11::value_error("Invalid eminfo.") );
//
//     const MultiID hor3dkey = ioobj_->key();
//     RefMan<EM::EMObject> obj = EM::EMM().loadIfNotFullyLoaded(hor3dkey);
//     if (!obj)
// 	throw( pybind11::value_error("Invalid emobject.") );
//
//     mDynamicCastGet(EM::Horizon3D*,hor,obj.ptr());
//     if (!hor)
// 	throw( pybind11::value_error("Invalid Horizon3D.") );
//
//     PtrMan<Array2D<float>> array = hor->createArray2D();
//
//     const int nx = tk_.nrLines();
//     const int ny = tk_.nrTrcs();
//     py::array_t<float> img( {nx, ny} );
//     auto r_img = img.mutable_unchecked<2>();
//     const float zfac = SI().showZ2UserFactor();
//     const float znan = std::nanf("");
//     for ( int iln=0; iln<nx; iln++ )
//     {
// 	for ( int xln = 0; xln<ny; xln++ )
// 	{
// 	    float z = array->get( iln, xln);
// 	    if ( mIsUdf(z) )
// 		z = znan;
// 	    else
// 		z *= zfac;
//
// 	    r_img(iln,xln) = z;
// 	}
//     }
//
//     py::list dims;
//     dims.append( "iline" );
//     dims.append( "xline" );
//     py::dict coords = getCoords( dims );
//     py::dict attribs;
//     attribs["description"] = getName();
//     attribs["units"] = SI().getZUnitString( false );
//     attribs["crs"] = survey_.get_epsgCode();
//
//     return XDA( img, "coords"_a=coords, "dims"_a=dims, "name"_a=getName(),
// 		"attrs"_a=attribs );
// }
//
//
// py::dict odHorizon3D::getCoords( const py::list& dims ) const
// {
//     auto XDA = py::module::import("xarray").attr("DataArray");
//     py::dict coords;
//     const int nl = tk_.nrLines();
//     const int nt = tk_.nrTrcs();
//     py::array_t<double> xpos( {nl, nt} );
//     py::array_t<double> ypos( {nl, nt} );
//     py::array_t<int> lines( nl );
//     py::array_t<int> trcs( nt );
//     auto r_xpos = xpos.mutable_unchecked<2>();
//     auto r_ypos = ypos.mutable_unchecked<2>();
//     auto r_lines = lines.mutable_unchecked<1>();
//     auto r_trcs = trcs.mutable_unchecked<1>();
//
//     for ( int inl=0; inl<nl; inl++ )
//     {
// 	const int line = tk_.lineID( inl );
// 	r_lines( inl ) = line;
// 	for ( int xln = 0; xln<nt; xln++ )
// 	{
// 	    const int trc = tk_.traceID( xln );
// 	    r_trcs( xln ) = trc;
// 	    const BinID bid( line, trc );
// 	    const Coord pos = tk_.toCoord( bid );
// 	    r_xpos( inl, xln ) = pos.x;
// 	    r_ypos( inl, xln ) = pos.y;
// 	}
//     }
//     py::dict xyattrs;
//     xyattrs["units"] = SI().getXYUnitString( false );
//     coords["iline"] = lines;
//     coords["xline"] = trcs;
//     coords["x"] = XDA( xpos, "dims"_a=dims,  "attrs"_a=xyattrs );
//     coords["y"] = XDA( ypos, "dims"_a=dims,  "attrs"_a=xyattrs );
//
//     return coords;
// }
//
//
// void odHorizon3D::putData( const py::object& pyobj, bool bycoord )
// {
//     survey_.activate();
//     auto XDA = py::module::import("xarray").attr("DataArray");
//     if ( forread_ || !array_ )
// 	throw( pybind11::value_error("cannot save, object is read only") );
//
//     if ( !isinstance(pyobj, py::type::of(XDA())) )
// 	throw( pybind11::type_error("input is not an xarray DataArray.") );
//
//     auto crs = pyobj.attr( "attrs" )["crs" ].cast<std::string>();
//     if ( bycoord && crs != survey_.get_epsgCode() )
//     {
// 	py::print("Input CRS: ", crs, "Survey CRS: ", survey_.get_epsgCode() );
// 	throw( pybind11::value_error("output by Coord requires same CRS.") );
//     }
//
//     py::tuple dims = pyobj.attr( "dims" ).cast<py::tuple>();
//     const bool is2d = dims.size()==1;
//     py::array_t<float> data = pyobj;
//     auto coords = pyobj.attr("coords");
//     py::array_t<double> xs = coords.attr("get")("x").attr("to_numpy")();
//     py::array_t<double> ys = coords.attr("get")("y").attr("to_numpy")();
//     py::array_t<int> linenrs =
//				coords.attr("get")("iline").attr("to_numpy")();
//     py::array_t<int> trcnrs = coords.attr("get")("xline").attr("to_numpy")();
//     const int nx = linenrs.size();
//     const int ny = trcnrs.size();
//
//     auto r_data = data.unchecked();
//     auto r_xs = xs.unchecked();
//     auto r_ys = ys.unchecked();
//     auto r_linenrs = linenrs.unchecked();
//     auto r_trcnrs = trcnrs.unchecked();
//
//     TrcKey trckey;
//     trckey.setIs2D( false );
//     const float zfac = SI().showZ2UserFactor();
//     for ( int iln=0; iln<nx; iln++ )
//     {
// 	for ( int xln = 0; xln<ny; xln++ )
// 	{
// 	    if ( bycoord || is2d )
// 	    {
// 		const Coord pos( r_xs(iln, xln), r_ys(iln, xln) );
// 		trckey.setFrom( pos );
// 	    }
// 	    else
// 	    {
// 		trckey.setLineNr( r_linenrs(iln) );
// 		trckey.setTrcNr( r_trcnrs(xln) );
// 	    }
//
// 	    if ( tk_.includes(trckey) )
// 	    {
// 		float val = r_data( iln, xln );
// #ifdef __win__
// 		if ( !isnan(val) )
// #else
// 		if ( !std::isnan(val) )
// #endif
// 		{
// 		    val /= zfac;
// 		    array_->set( tk_.lineIdx(trckey.inl()),
// 				tk_.trcIdx(trckey.crl()), val );
// 		    writecount_++;
// 		}
// 	    }
// 	}
//     }
// }
//
//


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp())
{
    if ( !ioobj_ )
	return;

    EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
    {
	if ( !errmsg_.isEmpty() )
	    errmsg_.addNewLine();

	errmsg_.add( "Invalid eminfo." );
    }

 //    const MultiID hor2dkey = ioobj_->key();
 //    EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded(hor2dkey);
 // //    if (!obj)
	// // throw( pybind11::value_error("Invalid emobject.") );
 //
 //    mDynamicCast(EM::Horizon2D*, hor_, obj);
 // //    if (!hor_)
	// // throw( pybind11::value_error("Invalid Horizon2D.") );
}


odHorizon2D::odHorizon2D( const odSurvey& thesurvey, const char* name,
			  bool creategeom, bool overwrite )
    : odEMObject(thesurvey, name, sKeyTranslatorGrp(), overwrite)
    , creategeom_(creategeom)
{

}


odHorizon2D::~odHorizon2D()
{
    if ( !forread_ )
	close();
}


void odHorizon2D::close()
{
    survey_.activate();
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
}


void odHorizon2D::getInfo( OD::JSON::Object& jsobj ) const
{
    survey_.activate();
    jsobj.setEmpty();
    EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return;

    jsobj.set( "Name", getName() );
    jsobj.set( "NrAttribute", getNrAttributes() );
    jsobj.set( "NrLines", getNrLines() );
    jsobj.set( "Z Range", eminfo.getZRange() );
}


BufferStringSet* odHorizon2D::getLineNames() const
{
    survey_.activate();
    BufferStringSet* names = nullptr;
    EM::IOObjInfo eminfo( ioobj_ );
    if ( !eminfo.isOK() )
	return names;

    names = new BufferStringSet;
    eminfo.getLineNames( *names );
    return names;
}


int odHorizon2D::getNrLines() const
{
    survey_.activate();
    TypeSet<Pos::GeomID> geomids;
    EM::IOObjInfo eminfo( ioobj_ );
    if ( eminfo.isOK() )
	eminfo.getGeomIDs( geomids );

    return geomids.size();
}


void odHorizon2D::getLineIDs( int num, int* ids ) const
{
    survey_.activate();
    EM::IOObjInfo eminfo( ioobj_ );
    if ( eminfo.isOK() )
    {
	TypeSet<Pos::GeomID> geomids;
	eminfo.getGeomIDs( geomids );
	for ( int idx=0; idx<num; idx++ )
	    if( geomids.validIdx(idx) )
		ids[idx] = geomids[idx].asInt();
    }
}


// py::object odHorizon2D::getData( int lineid ) const
// {
//     if ( !forread_ || !hor_ )
// 	throw( pybind11::value_error("horizon only opened for writing.") );
//
//     survey_.activate();
//     auto XDA = py::module::import("xarray").attr("DataArray");
//     Pos::GeomID geomid( lineid );
//     if ( !hor_->geometry().hasLine(geomid) )
// 	throw( pybind11::value_error("lineid not found.") );
//
//     const StepInterval<int> trcrg = hor_->geometry().colRange( geomid );
//     const int ntrc = trcrg.nrSteps()+1;
//     py::array_t<float> zs( ntrc );
//     py::array_t<int> trcnrs( ntrc );
//     py::array_t<double> xpos( ntrc );
//     py::array_t<double> ypos( ntrc );
//
//     auto r_zs = zs.mutable_unchecked<1>();
//     auto r_xpos = xpos.mutable_unchecked<1>();
//     auto r_ypos = ypos.mutable_unchecked<1>();
//     auto r_trcnrs = trcnrs.mutable_unchecked<1>();
//
//     const float zfac = SI().showZ2UserFactor();
//     const float znan = std::nanf("");
//     TrcKey tk( geomid, -1 );
//     for ( int trcdx=0; trcdx<ntrc; trcdx++ )
//     {
// 	const int trcnr = trcrg.atIndex( trcdx );
// 	tk.setTrcNr( trcnr );
// 	float z = hor_->getZ( tk );
// 	if ( mIsUdf(z) )
// 	    z = znan;
// 	else
// 	    z *= zfac;
//
// 	const Coord coord = hor_->getCoord( tk );
// 	r_zs( trcdx ) = z;
// 	r_trcnrs( trcdx ) = trcnr;
// 	r_xpos( trcdx ) =coord.x;
// 	r_ypos( trcdx ) =coord.y;
//     }
//
//     py::list dims;
//     py::dict coords;
//     dims.append( "trace" );
//     const int lineidx = hor_->geometry().lineIndex( geomid );
//     coords["linename"] = hor_->geometry().lineName( lineidx );
//     coords["horizon"] = hor_->name().buf();
//     coords["line"] = lineid;
//     coords["trace"] = trcnrs;
//     py::dict xyattrs;
//     xyattrs["units"] = SI().getXYUnitString( false );
//     coords["x"] = XDA( xpos, "dims"_a=dims,  "attrs"_a=xyattrs );
//     coords["y"] = XDA( ypos, "dims"_a=dims,  "attrs"_a=xyattrs );
//
//     py::dict attribs;
//     std::string name = coords["linename"].cast<std::string>() + ":"
// 					+ coords["horizon"].cast<std::string>();
//     attribs["description"] = name;
//     attribs["units"] = SI().getZUnitString( false );
//     attribs["crs"] = survey_.get_epsgCode();
//
//     return XDA( zs, "coords"_a=coords, "dims"_a=dims, "name"_a=name,
// 		"attrs"_a=attribs );
// }
//
//
// py::dict odHorizon2D::getData( const py::tuple& tup ) const
// {
//     TypeSet<int> lineids;
//     BufferStringSet linenms;
//     for ( int lidx=0; lidx<tup.size(); lidx++ )
//     {
// 	int lineid = -1;
// 	BufferString linenm;
// 	if ( py::isinstance<py::int_>(tup[lidx]) )
// 	{
// 	    lineid = tup[lidx].cast<int>();
// 	    Pos::GeomID geomid( lineid );
// 	    const int idx = hor_->geometry().lineIndex( geomid );
// 	    if ( hor_->geometry().hasLine( geomid ) )
// 		linenm = hor_->geometry().lineName( idx );
// 	    else
// 		lineid = -1;
// 	}
// 	else if ( py::isinstance<py::str>(tup[lidx]) )
// 	{
// 	    linenm = tup[lidx].cast<std::string>();
// 	    const int lineidx = hor_->geometry().lineIndex( linenm.buf() );
// 	    if ( lineidx!=-1 )
// 		lineid = hor_->geometry().geomID( lineidx ).asInt();
// 	    else
// 		linenm.setEmpty();
//
// 	}
// 	if ( lineid!=-1 && !linenm.isEmpty() )
// 	{
// 	    lineids += lineid;
// 	    linenms.add( linenm );
// 	}
//     }
//
//     py::dict res;
//     for ( int idx=0; idx<lineids.size(); idx++ )
// 	res[linenms.get(idx).buf()] = getData( lineids[idx] );
//
//     return res;
// }
//
//
void odHorizon2D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
}


// Horizon3D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Horizon3D, horizon3d)
hHorizon3D horizon3d_newout( hSurvey survey, const char* name,
			     const intStepInterval il,
			     const intStepInterval xl, bool overwrite )
{
    const auto* p = reinterpret_cast<odSurvey*>(survey);
    return new odHorizon3D( *p, name,
			    StepInterval<int>(il.start, il.stop, il.step),
			    StepInterval<int>(xl.start, xl.stop, xl.step),
			    overwrite  );
}

int horizon3d_attribcount( hHorizon3D self )
{
    const auto* p = reinterpret_cast<odHorizon3D*>(self);
    return p->getNrAttributes();
}

hStringSet horizon3d_attribnames( hHorizon3D self )
{
    const auto* p = reinterpret_cast<odHorizon3D*>(self);
    return p->getAttribNames();
}


// Horizon2D bindings
//------------------------------------------------------------------------------
mDefineBaseBindings(Horizon2D, horizon2d)
hHorizon2D horizon2d_newout( hSurvey survey, const char* name,
			     bool creategeom, bool overwrite )
{
    const auto* p = reinterpret_cast<odSurvey*>(survey);
    return new odHorizon2D( *p, name, creategeom, overwrite  );
}

int horizon2d_attribcount( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p->getNrAttributes();
}

hStringSet horizon2d_attribnames( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p->getAttribNames();
}

int horizon2d_linecount( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p->getNrLines();
}

void horizon2d_lineids( hHorizon2D self, int num, int* ids )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    p->getLineIDs( num, ids );
}

hStringSet horizon2d_linenames( hHorizon2D self )
{
    const auto* p = reinterpret_cast<odHorizon2D*>(self);
    return p->getLineNames();
}
