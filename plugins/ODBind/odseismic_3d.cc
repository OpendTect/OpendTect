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
#include "odseismic_3d.h"
#include "odsurvey.h"

#include "ioman.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "segydirecttr.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "tracedata.h"


mDefineEnumUtils(odSeismic3D, Seis3DFormat, "Output format/translator")
    {"CBVS", mSEGYDirectTranslNm, nullptr };


odSeismic3D::odSeismic3D( const odSurvey& thesurvey, const char* name )
    : odSeismicObject(thesurvey, name, sKeyTranslatorGrp())
{
    survey_.activate();
    SeisIOObjInfo seisinfo( name_, Seis::GeomType::Vol );
    seisinfo.getRanges( tkz_ );
}


odSeismic3D::odSeismic3D( const odSurvey& survey, const char* name,
			  Seis3DFormat fmt, bool overwrite )
    : odSeismicObject(survey, name, sKeyTranslatorGrp(), overwrite,
		      toString(fmt))
{
    tkz_.setEmpty();
    const Seis::GeomType gt = Seis::GeomType::Vol;
    writer_ = new SeisTrcWriter( *ioobj_, &gt );
}


odSeismic3D::odSeismic3D( const odSeismic3D& oth )
    : odSeismicObject(oth)
    , tkz_(oth.tkz_)
{}



odSeismic3D::~odSeismic3D()
{
}


int odSeismic3D::getNrTraces() const
{
    survey_.activate();
    SeisIOObjInfo::SpaceInfo si;
    SeisIOObjInfo seisinfo( name_, Seis::GeomType::Vol );
    seisinfo.getDefSpaceInfo( si );
    return si.expectednrtrcs;
}


void odSeismic3D::getInfo( OD::JSON::Object& jsobj ) const
{
    survey_.activate();
    jsobj.setEmpty();
    jsobj.set( "name", getName().buf() );
    jsobj.set( "inl_range", tkz_.hsamp_.lineRange() );
    jsobj.set( "crl_range", tkz_.hsamp_.trcRange() );
    jsobj.set( "z_range", tkz_.zsamp_ );
    jsobj.set( "comp_count", getNrComponents() );
    jsobj.set( "storage_dtype", getDtypeStr().buf() );
    jsobj.set( "nrsamp", tkz_.nrZ() );
    jsobj.set( "bin_count", tkz_.hsamp_.totalNr() );
    jsobj.set( "trc_count", getNrTraces() );
}


void odSeismic3D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    survey_.activate();
    TypeSet<Coord> coords;
    TrcKeySampling tk = tkz_.hsamp_;
    for ( int i=0; i<4; i++ )
	coords += tk.toCoord( tk.corner(i) );

    coords.swap( 2, 3 );
    coords += coords[0];
    survey_.makeCoordsList( jsarr, coords, towgs );
}


mDefineBaseBindings(Seismic3D, seismic3d)

hStringSet seismic3d_compnames( hSeismic3D self )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return nullptr;

    return p->getCompNames();
}


void seismic3d_getinlcrl( hSeismic3D self, size_t traceidx, int* inl, int* crl )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return;

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() || traceidx<0 || traceidx>=tkz.hsamp_.totalNr() )
    {
	p->setErrMsg( "invalid trace index" );
	return;
    }

    const TrcKey pos = tkz.hsamp_.trcKeyAt( traceidx );
    *inl = pos.lineNr();
    *crl = pos.trcNr();
}


size_t seismic3d_gettrcidx( hSeismic3D self, int iln, int crl )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    const TrcKey trckey( BinID( iln, crl ) );
    if ( tkz.isEmpty() || !tkz.hsamp_.includes(trckey) )
	return -1;

    return tkz.hsamp_.globalIdx( trckey );
}


size_t seismic3d_nrbins( hSeismic3D self )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() )
	return -1;

    return tkz.hsamp_.totalNr();
}


size_t seismic3d_nrtrcs( hSeismic3D self )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    return p->getNrTraces();
}




