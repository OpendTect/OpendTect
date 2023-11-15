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
#include "odgeom_2d.h"
#include "odsurvey.h"
#include "ioman.h"
#include "posinfo2d.h"
#include "survgeom2d.h"



odGeom2D::odGeom2D( const odSurvey& thesurvey, const char* name )
    : odSurveyObject(thesurvey, name, translatorGrp())
{
}


odGeom2D::odGeom2D( const odSurvey& survey, const char* name, bool overwrite )
    : odSurveyObject(survey, name, nullptr, overwrite)
{
    Pos::GeomID geomid = Survey::GM().getGeomID( name );
    if (  !geomid.isValid() )
    {
	auto* l2d = new PosInfo::Line2DData( name );
	auto* newgeom = new Survey::Geometry2D( l2d );
	uiString msg;
	geomid = Survey::GMAdmin().addNewEntry( newgeom, msg );
	if ( !Survey::is2DGeom(geomid) )
	    errmsg_ = BufferString( "Cannot add new geometry: ",
				    msg.getString() );
    }
    else if ( overwrite )
    {
	mDynamicCastGet( Survey::Geometry2D*, geom2d,
			 Survey::GMAdmin().getGeometry(geomid) );
	if ( geom2d )
	{
	    geom2d->dataAdmin().setEmpty();
	    geom2d->touch();
	}
	else
	    errmsg_ = "Can only overwrite a 2d geometry.";
    }
    else
	errmsg_ = "Object already exists and overwrite is disabled.";

    ioobj_ = IOM().get( name, translatorGrp() );
    if ( !ioobj_ )
	errmsg_.insertAt( 0, "IO object creation error: " );
    else
	name_ = ioobj_->name();

}


odGeom2D::~odGeom2D()
{
    close();
}


void odGeom2D::close()
{
}


void odGeom2D::getData( hAllocator allocator ) const
{
    errmsg_.setEmpty();
    if ( !canRead() )
	return;

    survey_.activate();
    if ( !ioobj_ )
    {
	errmsg_ = "invalid ioobj.";
	return;
    }

    const Pos::GeomID geomid = Survey::GM().getGeomID( getName().buf() );
    if ( !geomid.is2D() )
    {
	errmsg_ = "invalid data request.";
	return;
    }

    const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomid );
    const TypeSet<PosInfo::Line2DPos>& pos = geom2d.data().positions();
    const TypeSet<float>& sps = geom2d.spnrs();
    const int sz = pos.size();

    const int ndim = 1;
    PtrMan<int> dims = new int[ndim];
    dims[0] = sz;
    int* trcnrs = static_cast<int*>(allocator(ndim, dims, 'i'));
    float* spnrs = static_cast<float*>(allocator(ndim, dims, 'f'));
    double* xdata = static_cast<double*>(allocator(ndim, dims, 'd'));
    double* ydata = static_cast<double*>(allocator(ndim, dims, 'd'));
    for ( int idx=0; idx<sz; idx++ )
    {
	*trcnrs++ = pos[idx].nr_;
	*spnrs++ = sps[idx];
	*xdata++ = pos[idx].coord_.x;
	*ydata++ = pos[idx].coord_.y;
    }
}


void odGeom2D::putData( int32_t numpos, const int32_t* trcnrs,
			const float* spnrs, const double* xpos,
			const double* ypos)
{
    errmsg_.setEmpty();
    if ( !canWrite() )
	return;

    survey_.activate();
    if ( !ioobj_ )
    {
	errmsg_ = "invalid ioobj.";
	return;
    }
    const Pos::GeomID geomid = Survey::GM().getGeomID( getName().buf() );
    if ( !geomid.is2D() )
    {
	errmsg_ = "invalid data request.";
	return;
    }

    RefMan<Survey::Geometry2D> geom2d;
    mDynamicCast(Survey::Geometry2D*,geom2d, Survey::GMAdmin().getGeometry(
									geomid))
    for ( int idx=0; idx<numpos; idx++ )
    {
	Coord pos( xpos[idx], ypos[idx] );
	geom2d->add( pos, trcnrs[idx], spnrs[idx] );
    }
    geom2d->touch();

    uiString errmsg;
    if ( !Survey::GMAdmin().write(*geom2d,errmsg) )
	errmsg_ = errmsg.getString();

}


void odGeom2D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    survey_.activate();
    const Pos::GeomID geomid = Survey::GM().getGeomID( getName().buf() );
    if ( !geomid.is2D() )
	return;

    const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomid );
    const TypeSet<PosInfo::Line2DPos>& pos = geom2d.data().positions();
    const TypeSet<float>& sps = geom2d.spnrs();
    const int sz = pos.size();

    jsobj.set( "name", getName().buf() );
    jsobj.set( "trc_range", Interval<int>(pos[0].nr_, pos[sz-1].nr_) );
    jsobj.set( "sp_range", Interval<float>(sps[0], sps[sz-1]) );
    jsobj.set( "line_length", geom2d.lineLength() );
    jsobj.set( "average_trace_distance", geom2d.averageTrcDist() );
}


void odGeom2D::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
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


void odGeom2D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    const Pos::GeomID geomid = Survey::GM().getGeomID( getName().buf() );
    if ( !geomid.is2D() )
	return;

    const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomid );
    TypeSet<PosInfo::Line2DPos> bendpts;
    geom2d.data().getBendPositions( bendpts );
    TypeSet<Coord> coords;
    for ( auto& pt : bendpts )
	coords += pt.coord_;

    auto* jscoords = new OD::JSON::Array( false );
    survey_.makeCoordsList( *jscoords, coords, towgs );
    jsarr.add( jscoords );
}


mDefineBaseBindings(Geom2D, geom2d)


hGeom2D geom2d_newout( hSurvey survey, const char* name, bool overwrite )
{
    auto* p = static_cast<odSurvey*>(survey);
    if ( !p )
	return nullptr;

    return new odGeom2D( *p, name, overwrite );
}


void geom2d_close( hGeom2D self )
{
    auto* p = static_cast<odGeom2D*>(self);
    if  ( !p )
	return;

    p->close();
}


void geom2d_get( hGeom2D self, hAllocator allocator )
{
    const auto* p = static_cast<odGeom2D*>(self);
    if  ( !p || !p->canRead() )
	return;

    p->getData( allocator );
}


void geom2d_put( hGeom2D self, int32_t numpos, const int32_t* trcnrs,
		 const float* spnrs, const double* xpos, const double* ypos )
{
    auto* p = static_cast<odGeom2D*>(self);
    if  ( !p || !p->canWrite() )
	return;

    p->putData( numpos, trcnrs, spnrs, xpos, ypos );
}


void geom2d_removeobjs( hSurvey survey, hStringSet objnms )
{
    const auto* p = static_cast<odSurvey*>(survey);
    const auto* nms = static_cast<BufferStringSet*>(objnms);
    if ( !p || !nms )
	return;

    for ( const auto* nm : *nms )
    {
	const Pos::GeomID geomid = Survey::GM().getGeomID( *nm );
	Survey::GMAdmin().removeGeometry( geomid );
    }

    odGeom2D::removeObjects<odGeom2D>( *p, *nms );
}

