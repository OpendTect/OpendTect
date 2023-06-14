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
#include "odseismic_2d.h"
#include "odsurvey.h"
#include "posinfo2d.h"
#include "segydirecttr.h"
#include "seisioobjinfo.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "survgeom2d.h"



mDefineEnumUtils(odSeismic2D, Seis2DFormat, "Output format/translator")
    {"CBVS", mSEGYDirectTranslNm, nullptr };


odSeismic2D::odSeismic2D( const odSurvey& thesurvey, const char* name )
    : odSeismicObject(thesurvey, name, translatorGrp())
    , seisdata_(new Seis2DDataSet(*ioobj_))
{
}


odSeismic2D::odSeismic2D( const odSurvey& survey, const char* name,
			  Seis2DFormat fmt, const BufferStringSet& components,
			  bool zistime, bool overwrite )
    : odSeismicObject(survey, name, components, translatorGrp(), toString(fmt),
		      zistime, overwrite)
    , seisdata_(new Seis2DDataSet(*ioobj_))
{
}


odSeismic2D::~odSeismic2D()
{
    close();
}


void odSeismic2D::close()
{
}


int odSeismic2D::getNrLines() const
{
    return seisdata_->nrLines();
}


BufferStringSet* odSeismic2D::getLineNames() const
{
    auto* nms = new BufferStringSet;
    seisdata_->getLineNames( *nms );
    return nms;
}


void odSeismic2D::getLineInfo( OD::JSON::Array& jsarr,
			       const BufferStringSet& fornames ) const
{
    survey_.activate();
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getLineNames();
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = odSurvey::getCommonItems( *allnms, fornames );

    jsarr.setEmpty();
    const SeisIOObjInfo info( ioobj_ );
    const ZDomain::Def& zdef = info.zDomainDef();
    for ( const auto* nm : nms )
    {
	int lidx = seisdata_->indexOf( nm->buf() );
	if ( lidx<0 )
	    continue;

	const auto geomid = seisdata_->geomID( lidx );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	seisdata_->getRanges( geomid, trcrg, zrg );
	zrg.scale( zdef.userFactor() );
	OD::JSON::Object lineinfo;
	lineinfo.set( "name", nm->buf() );
	lineinfo.set( "trc_range", trcrg );
	lineinfo.set( "z_range", zrg );
	jsarr.add( lineinfo.clone() );
    }
}



void odSeismic2D::getData( hAllocator allocator, const char* linenm,
			   float zrg[3] ) const
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

    if ( !seisdata_->isPresent(linenm) )
    {
	errmsg_ = "invalid data request.";
	return;
    }

    Pos::GeomID geomid = seisdata_->geomID( seisdata_->indexOf(linenm) );
    SeisTrcBuf tbuf( true );
    PtrMan<Executor>ex = seisdata_->lineFetcher( geomid, tbuf );
    if ( !ex->execute() || tbuf.isEmpty() )
    {
	errmsg_ = "error reading 2D seismic data.";
	return;
    }

    const int ntrc = tbuf.size();
    const int nsamp = tbuf.get(0)->size();
    const int nrcomp = getNrComponents();
    StepInterval<float> zrange = tbuf.get(0)->zRange();
    const SeisIOObjInfo info( ioobj_ );
    const ZDomain::Def& zdef = info.zDomainDef();
    zrange.scale( zdef.userFactor() );
    zrg[0] = zrange.start;
    zrg[1] = zrange.stop;
    zrg[2] = zrange.step;

    const int ndim = 2;
    PtrMan<int> dims = new int[ndim];
    dims[0] = ntrc;
    dims[1] = nsamp;
    const float valnan = std::nanf("");
    for ( int cidx=0; cidx<nrcomp; cidx++ )
    {
	float* outdata = reinterpret_cast<float*>( allocator(ndim, dims, 'f') );
	for ( int tidx=0; tidx<ntrc; tidx++ )
	{
	    const SeisTrc* trc = tbuf.get( tidx );
	    for ( int zidx=0; zidx<nsamp; zidx++ )
	    {
		float val = trc->get( zidx, cidx );
		if ( mIsUdf(val) )
		    val = valnan;

		*outdata++ = val;
	    }
	}
    }

    const int ndim_xy = 1;
    PtrMan<int> dims_xy = new int[ndim_xy];
    dims_xy[0] = tbuf.size();
    int* trcdata = reinterpret_cast<int*>(allocator(ndim_xy, dims_xy, 'i'));
    float* refdata = reinterpret_cast<float*>(allocator(ndim_xy, dims_xy, 'f'));
    double* xdata = reinterpret_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    double* ydata = reinterpret_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    for ( int tidx=0; tidx<ntrc; tidx++ )
    {
	const SeisTrcInfo& trc = tbuf.get( tidx )->info();
	*trcdata++ = trc.trcNr();
	*refdata++ = trc.refnr;
	*xdata++ = trc.coord.x;
	*ydata++ = trc.coord.y;
    }
}


void odSeismic2D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    survey_.activate();
    const SeisIOObjInfo seisinfo( ioobj_ );
    const ZDomain::Def& zdef = seisinfo.zDomainDef();
    jsobj.set( "name", getName().buf() );
    jsobj.set( "line_count", getNrLines() );
    jsobj.set( "zunit", zdef.unitStr() );
    jsobj.set( "comp_count", getNrComponents() );
    jsobj.set( "storage_dtype", getDtypeStr(seisinfo).buf() );
}


void odSeismic2D::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
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


void odSeismic2D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    TypeSet<Pos::GeomID> geomids;
    seisdata_->getGeomIDs( geomids );
    for ( auto& geomid : geomids )
    {
	TypeSet<Coord> coords;
	const auto& geom2d = Survey::GM().get2D( geomid );
	coords += geom2d.data().positions().first().coord_;
	coords += geom2d.data().positions().last().coord_;
	auto* jscoords = new OD::JSON::Array( false );
	survey_.makeCoordsList( *jscoords, coords, towgs );
	jsarr.add( jscoords );
    }
}


mDefineBaseBindings(Seismic2D, seismic2d)


void seismic2d_close( hSeismic2D self )
{
    auto* p = reinterpret_cast<odSeismic2D*>(self);
    if  ( !p )
	return;

    p->close();
}


hStringSet seismic2d_compnames( hSeismic2D self )
{
    const auto* p = reinterpret_cast<odSeismic2D*>(self);
    if  ( !p )
	return nullptr;

    return p->getCompNames();
}


hStringSet seismic2d_linenames( hSeismic2D self )
{
    const auto* p = reinterpret_cast<odSeismic2D*>(self);
    if  ( !p )
	return nullptr;

    return p->getLineNames();
}


const char* seismic2d_lineinfo( hSeismic2D self, const hStringSet fornms )
{
    auto* p = reinterpret_cast<odSeismic2D*>(self);
    const auto* nms = reinterpret_cast<BufferStringSet*>(fornms);
    if ( !p || !nms ) return nullptr;
    OD::JSON::Array jsarr( true );
    p->getLineInfo( jsarr, *nms );
    return strdup( jsarr.dumpJSon().buf() );
}


void seismic2d_getdata( hSeismic2D self, hAllocator allocator,
			const char* linenm, float zrg[3] )
{
    const auto* p = reinterpret_cast<odSeismic2D*>(self);
    if  ( !p || !p->canRead() )
	return;

    p->getData( allocator, linenm, zrg );
}



