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
    errmsg_.setEmpty();
    survey_.activate();
    SeisIOObjInfo seisinfo( name_, Seis::GeomType::Vol );
    seisinfo.getRanges( tkz_ );
}


odSeismic3D::odSeismic3D( const odSurvey& survey, const char* name,
			  Seis3DFormat fmt, bool overwrite )
    : odSeismicObject(survey, name, sKeyTranslatorGrp(), overwrite,
		      toString(fmt))
{
    errmsg_.setEmpty();
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


void odSeismic3D::getData( hAllocator allocator,
			   const TrcKeyZSampling& tkzforload )
{
    errmsg_.setEmpty();
    survey_.activate();
    if ( !ioobj_ )
    {
	errmsg_ = "invalid ioobj.";
	return;
    }

    if ( tkzforload.isEmpty() || !tkz_.includes(tkzforload) )
    {
	errmsg_ = "invalid data request.";
	return;
    }

    Seis::SequentialReader rdr( *ioobj_, &tkzforload );
    if ( !rdr.execute() )
    {
	errmsg_ = "reading seismic volume failed.";
	return;
    }

    ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
    const TrcKeyZSampling& tkz = dp->sampling();
    const int nrcomp = getNrComponents();
    const int ndim = dp->nrTrcs()==1 ? 1 : (tkz.isFlat() ? 2 : 3);
    const bool iszslice = ndim==2 && tkz.nrZ()==1;
    PtrMan<int> dims = new int[ndim];
    if ( ndim==1 )
	dims[0] = tkz.nrZ();
    else if ( ndim==2 )
    {
	if ( iszslice )
	{
	    dims[0] = tkz.nrLines();
	    dims[1] = tkz.nrTrcs();
	}
	else
	{
	    dims[0] = dp->nrTrcs();
	    dims[1] = tkz.nrZ();
	}
    }
    else
    {
	dims[0] = tkz.nrLines();
	dims[1] = tkz.nrTrcs();
	dims[2] = tkz.nrZ();
    }

    const float valnan = std::nanf("");
    for ( int cidx=0; cidx<nrcomp; cidx++ )
    {
	const auto array = dp->data( cidx );
	float* outdata = reinterpret_cast<float*>( allocator(ndim, dims, 'f') );
	const float* indata = array.getData();
	for ( size_t idx=0; idx<array.totalSize(); idx++)
	{
	    const float val = *indata++;
	    *outdata++ = mIsUdf(val) ? valnan : val;
	}
    }
    const int ndim_xy = ndim==3 || iszslice ? 2 : 1;
    PtrMan<int> dims_xy = new int[ndim_xy];
    if ( ndim_xy==1 )
	dims_xy[0] = dp->nrTrcs();
    else
    {
	dims_xy[0] = tkz.nrLines();
	dims_xy[1] = tkz.nrTrcs();
    }
    double* xdata = reinterpret_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    double* ydata = reinterpret_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    for ( int idx=0; idx<dp->nrTrcs(); idx++ )
    {
	const TrcKey trckey = dp->getTrcKey( idx );
	const Coord pos = trckey.getCoord();
	*xdata++ = pos.x;
	*ydata++ = pos.y;
    }
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


int seismic3d_getzidx( hSeismic3D self, float zval )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() || !tkz.zsamp_.includes(zval, false) )
    {
	p->setErrMsg( "invalid z value location.");
	return -1;
    }

    return tkz.zIdx( zval );
}


float seismic3d_getzval( hSeismic3D self, int zidx )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return std::nanf("");

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() || zidx<0 || zidx>tkz.nrZ() )
    {
	p->setErrMsg( "invalid z index.");
	return std::nanf("");
    }

    return tkz.zAtIndex( zidx );
}


od_int64 seismic3d_gettrcidx( hSeismic3D self, int iln, int crl )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    const TrcKey trckey( BinID( iln, crl ) );
    if ( tkz.isEmpty() || !tkz.hsamp_.includes(trckey) )
    {
	p->setErrMsg( "invalid iln,crl location.");
	return -1;
    }

    return tkz.hsamp_.globalIdx( trckey );
}


od_int64 seismic3d_nrbins( hSeismic3D self )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() )
	return -1;

    return tkz.hsamp_.totalNr();
}


od_int64 seismic3d_nrtrcs( hSeismic3D self )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    return p->getNrTraces();
}


void seismic3d_getdata( hSeismic3D self, hAllocator allocator,
			const int inl_rg[3], const int crl_rg[3],
			const int z_rg[3] )
{
    auto* p = reinterpret_cast<odSeismic3D*>(self);
    if  ( !p )
	return;

    const auto& tkz = p->tkz();
    StepInterval<int> linerg( inl_rg[0], inl_rg[1], inl_rg[2] );
    linerg.limitTo( tkz.hsamp_.lineRange() );
    StepInterval<int> trcrg( crl_rg[0], crl_rg[1], crl_rg[2] );
    trcrg.limitTo( tkz.hsamp_.trcRange() );
    StepInterval<float> zrg( tkz.zAtIndex(z_rg[0]), tkz.zAtIndex(z_rg[1]-1),
			     z_rg[2]*tkz.zsamp_.step );
    zrg.limitTo( tkz.zsamp_ );
    TrcKeyZSampling tkztoload;
    tkztoload.hsamp_.setLineRange( linerg );
    tkztoload.hsamp_.setTrcRange( trcrg );
    tkztoload.zsamp_ = zrg;
    p->getData( allocator, tkztoload );
}




