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
#include "cubedataidx.h"

#include "ioman.h"
#include "ioobj.h"
#include "posinfo.h"
#include "seisdatapack.h"
#include "segydirecttr.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seistrc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "tracedata.h"



mDefineEnumUtils(odSeismic3D, Seis3DFormat, "Output format/translator")
    {"CBVS", mSEGYDirectTranslNm, nullptr };


odSeismic3D::odSeismic3D( const odSurvey& thesurvey, const char* name )
    : odSeismicObject(thesurvey, name, translatorGrp())
{
    const Seis::GeomType gt = Seis::GeomType::Vol;
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    SeisTrcReader rdr( ioobj->key(), gt );
    if ( !rdr.isOK() )
    {
	errmsg_ = "unable to open SeisTrcReader\n";
	errmsg_.add( rdr.errMsg().getString() );
	return;
    }

    PosInfo::CubeData cubedata;
    if ( !rdr.get3DGeometryInfo(cubedata) )
    {
	errmsg_ = "unable to access 3D geometry information.";
	return;
    }

    cubeidx_ = new PosInfo::CubeDataIndex( cubedata );
    const SeisIOObjInfo seisinfo( ioobj.ptr() );
    seisinfo.getRanges( tkz_ );
}


odSeismic3D::odSeismic3D( const odSurvey& survey, const char* name,
			  Seis3DFormat fmt, const BufferStringSet& components,
			  const TrcKeyZSampling& tkz,
			  bool zistime, bool overwrite )
    : odSeismicObject(survey, name, components, translatorGrp(), toString(fmt),
		      zistime, overwrite)
    , tkz_(tkz)
{
    const Seis::GeomType gt = Seis::GeomType::Vol;
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( ioobj )
	writer_ = new SeisTrcWriter( ioobj->key(), gt );

    if ( !writer_ || !writer_->isOK() )
    {
	errmsg_ = "unable to open SeisTrcWriter\n";
	if ( !writer_ )
	    errmsg_.add( writer_->errMsg().getString() );
	return;
    }

    const PosInfo::CubeData cubedata( tkz.hsamp_.start_, tkz.hsamp_.stop_,
				tkz.hsamp_.step_);
    cubeidx_ = new PosInfo::CubeDataIndex( cubedata );
}


odSeismic3D::~odSeismic3D()
{
    close();
}


void odSeismic3D::close()
{
    if ( sequentialwriter_ )
    {
	sequentialwriter_->finishWrite();
	sequentialwriter_ = nullptr;
    }

    if ( writer_ )
    {
	writer_ = nullptr;
	ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
	if ( writecount_==0 && ioobj )
	{
	    IOM().to( ioobj->key() );
	    IOM().implRemove( ioobj->key(), true );
	}

	writecount_ = 0;
    }
}


od_int64 odSeismic3D::getNrTraces() const
{
    return cubeidx_ ? cubeidx_->lastTrc()+1 : -1;
}


od_int64 odSeismic3D::getTrcNum( const BinID& bid ) const
{
    errmsg_.setEmpty();
    const od_int64 trcnum = cubeidx_ ? cubeidx_->trcNumber( bid ) : -1;
    if ( trcnum==-1 )
	errmsg_ = "invalid bin location.";

    return trcnum;
}


BinID odSeismic3D::getBinID( od_int64 trcnum ) const
{
    errmsg_.setEmpty();
    const BinID bid = cubeidx_ ? cubeidx_->binID( trcnum ) : BinID::udf();
    if ( bid.isUdf() )
	errmsg_ = "invalid trace number.";

    return bid;
}


StepInterval<float> odSeismic3D::getZrange() const
{
    StepInterval<float> res;
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( ioobj )
    {
	PtrMan<SeisIOObjInfo> info = new SeisIOObjInfo( ioobj.ptr() );
	res = getZrange( *info );
    }

    return res;
}


StepInterval<float> odSeismic3D::getZrange(const SeisIOObjInfo& info) const
{
    errmsg_.setEmpty();
    const ZDomain::Def& zdef = info.zDomainDef();
    StepInterval<float> zrg = tkz_.zsamp_;
    zrg.scale( zdef.userFactor() );
    return zrg;
}


int odSeismic3D::makeDims( const TrcKeyZSampling& tkz,
			   TypeSet<int>& dims ) const
{
    errmsg_.setEmpty();
    dims.setEmpty();
    const int total_trcs = tkz.hsamp_.totalNr();
    const int ndim = total_trcs==1 ? 1 : (tkz.isFlat() ? 2 : 3);
    const bool iszslice = ndim==2 && tkz.nrZ()==1;
    if ( ndim==1 )
	dims += tkz.nrZ();
    else if ( ndim==2 )
    {
	if ( iszslice )
	{
	    dims += tkz.nrLines();
	    dims += tkz.nrTrcs();
	}
	else
	{
	    dims += total_trcs;
	    dims += tkz.nrZ();
	}
    }
    else
    {
	dims += tkz.nrLines();
	dims += tkz.nrTrcs();
	dims += tkz.nrZ();
    }

    return ndim;
}


void odSeismic3D::getData( hAllocator allocator,
			   const TrcKeyZSampling& tkzforload ) const
{
    errmsg_.setEmpty();
    if ( !canRead() )
	return;

    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    if ( tkzforload.isEmpty() )
    {
	errmsg_ = "invalid data request.";
	return;
    }

    Seis::SequentialReader rdr( *ioobj, &tkzforload );
    if ( !rdr.execute() )
    {
	errmsg_ = "reading seismic volume failed.";
	return;
    }

    ConstRefMan<RegularSeisDataPack> dp = rdr.getDataPack();
    const TrcKeyZSampling& tkz = dp->sampling();
    const int nrcomp = getNrComponents();
    TypeSet<int> dims;
    const int ndim = makeDims( tkz, dims );
    const bool iszslice = ndim==2 && tkz.nrZ()==1;
    const float valnan = std::nanf("");
    for ( int cidx=0; cidx<nrcomp; cidx++ )
    {
	const auto array = dp->data( cidx );
	float* outdata = static_cast<float*>( allocator(ndim, dims.arr(),
							     'f') );
	const float* indata = array.getData();
	MemCopier<float> copier( outdata, indata, array.totalSize() );
	copier.execute();
	MemValReplacer<float> udfrepl( outdata, mUdf(float), valnan,
				   array.totalSize() );
	udfrepl.execute();
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
    double* xdata = static_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    double* ydata = static_cast<double*>(allocator(ndim_xy, dims_xy, 'd'));
    for ( int idx=0; idx<dp->nrTrcs(); idx++ )
    {
	const TrcKey trckey = dp->getTrcKey( idx );
	const Coord pos = trckey.getCoord();
	*xdata++ = pos.x;
	*ydata++ = pos.y;
    }
}


void odSeismic3D::putData( const float** data, const TrcKeyZSampling& tkz )
{
    errmsg_.setEmpty();
    if ( !canWrite() )
	return;

    if ( !writer_ )
    {
	errmsg_ = "no SeisTrcWriter.";
	return;
    }

    survey_.activate();
    if ( writecount_==0 )
    {
	writer_->setComponentNames( components_ );
	sequentialwriter_ = new SeisSequentialWriter( writer_ );
    }

    if ( !sequentialwriter_ )
    {
	errmsg_ = "no SeisSequentialWriter.";
	return;
    }

    const SamplingData<float> sd( tkz_.zsamp_ );
    const int nrcomp = getNrComponents();
    for ( od_int64 idx=0; idx<tkz.hsamp_.totalNr(); idx++ )
    {
	const TrcKey trckey = tkz.hsamp_.trcKeyAt( idx );
	if ( !tkz_.hsamp_.includes(trckey, false) )
	    continue;

	auto* trc = new SeisTrc( tkz_.nrZ() );
	trc->setNrComponents( nrcomp, DataCharacteristics::F32 );
	trc->info().setTrcKey( trckey );
	trc->info().coord = trckey.getCoord();
	trc->info().sampling = sd;
	for ( int icomp=0; icomp<nrcomp; icomp++ )
	{
	    const float* compdata = data[icomp];
	    const int idxstart = idx*tkz.nrZ();
	    trc->setAll( mUdf(float), icomp);
	    for ( int iz=0; iz<tkz_.nrZ(); iz++ )
	    {
		const int altiz = mNINT32( tkz.zsamp_.getfIndex(
							tkz_.zAtIndex(iz)) );
		float val = mUdf(float);
		if ( altiz>=0 && altiz<tkz.nrZ() )
		{
		    val = compdata[idxstart + altiz];
		    if ( !Math::IsNormalNumber(val) )
			val = mUdf(float);
		}

		trc->set( iz, val, icomp );
	    }
	}
	sequentialwriter_->announceTrace( trc->info().binID() );
	sequentialwriter_->submitTrace( trc, true );
	writecount_++;
    }
}


void odSeismic3D::getInfo( OD::JSON::Object& jsobj ) const
{
    jsobj.setEmpty();
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    const SeisIOObjInfo seisinfo( ioobj.ptr() );
    const ZDomain::Def& zdef = seisinfo.zDomainDef();
    jsobj.set( "name", getName().buf() );
    jsobj.set( "inl_range", tkz_.hsamp_.lineRange() );
    jsobj.set( "crl_range", tkz_.hsamp_.trcRange() );
    jsobj.set( "z_range", getZrange(seisinfo) );
    jsobj.set( "zunit", zdef.unitStr() );
    jsobj.set( "comp_count", getNrComponents() );
    jsobj.set( "storage_dtype", getDtypeStr(seisinfo).buf() );
    jsobj.set( "nrsamp", tkz_.nrZ() );
    jsobj.set( "bin_count", tkz_.hsamp_.totalNr() );
    jsobj.set( "trc_count", getNrTraces() );
}


void odSeismic3D::getPoints( OD::JSON::Array& jsarr, bool towgs ) const
{
    if ( !canRead() )
	return;

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
mDefineRemoveBindings(Seismic3D, seismic3d)


hSeismic3D seismic3d_newout( hSurvey survey, const char* name,
			     const char* format, hStringSet compnames,
			     const int32_t inlrg[3], const int32_t crlrg[3],
			     const float zrg[3], bool zistime, bool overwrite )
{
    auto* p = static_cast<odSurvey*>(survey);
    const auto* nms = static_cast<BufferStringSet*>(compnames);
    if ( !p || !nms ) return nullptr;

    TrcKeyZSampling tkz = odSurvey::tkzFromRanges( inlrg, crlrg, zrg, zistime );
    odSeismic3D::Seis3DFormat fmt;
    if ( odSeismic3D::parseEnum(format, fmt) )
	return new odSeismic3D( *p, name, fmt, *nms, tkz, zistime, overwrite );
    else
    {
	p->setErrMsg("invalid output format");
	return nullptr;
    }
}


void seismic3d_close( hSeismic3D self )
{
    auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return;

    p->close();
}


hStringSet seismic3d_compnames( hSeismic3D self )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return nullptr;

    return p->getCompNames();
}


void seismic3d_getinlcrl( hSeismic3D self, size_t traceidx, int32_t* inl,
			  int32_t* crl )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return;

    const BinID bid = p->getBinID( traceidx );
    *inl = bid.inl();
    *crl = bid.crl();
}


int seismic3d_getzidx( hSeismic3D self, float zval )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& zrg = p->getZrange();
    const int zidx = zrg.getIndex( zval );
    if ( zidx<0 || zidx>zrg.nrSteps() )
    {
	p->setErrMsg( "invalid z value location.");
	return -1;
    }

    return zidx;
}


float seismic3d_getzval( hSeismic3D self, int32_t zidx )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return std::nanf("");

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() || zidx<0 || zidx>=tkz.nrZ() )
    {
	p->setErrMsg( "invalid z index.");
	return std::nanf("");
    }

    const auto& zrg = p->getZrange();
    return zrg.atIndex( zidx );
}


od_int64 seismic3d_gettrcidx( hSeismic3D self, int32_t iln, int32_t crl )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const BinID bid( iln, crl );
    return p->getTrcNum( bid );
}


od_int64 seismic3d_nrbins( hSeismic3D self )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    const auto& tkz = p->tkz();
    if ( tkz.isEmpty() )
	return -1;

    return tkz.hsamp_.totalNr();
}


od_int64 seismic3d_nrtrcs( hSeismic3D self )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p )
	return -1;

    return p->getNrTraces();
}


void seismic3d_zrange( hSeismic3D self, float zrg[3] )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if ( !p ) return;

    const auto& z_rg = p->getZrange();
    zrg[0] = z_rg.start;
    zrg[1] = z_rg.stop;
    zrg[2] = z_rg.step;
}


bool seismic3d_validrange( hSeismic3D self, const int32_t inlrg[3],
			   const int32_t crlrg[3], const float zrg[3] )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if ( !p ) return false;

    TrcKeyZSampling tkztochk = odSurvey::tkzFromRanges( inlrg, crlrg, zrg,
							p->zIsTime() ) ;
    return p->tkz().includes( tkztochk );
}


void seismic3d_getdata( hSeismic3D self, hAllocator allocator,
			const int32_t inlrg[3], const int32_t crlrg[3],
			const float zrg[3] )
{
    const auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p || !p->canRead() )
	return;

    TrcKeyZSampling tkztoload = odSurvey::tkzFromRanges( inlrg, crlrg, zrg,
							 p->zIsTime() ) ;
    tkztoload.limitTo( p->tkz() );
    p->getData( allocator, tkztoload );
}


void seismic3d_putdata( hSeismic3D self,
			const float** data,
		        const int32_t inlrg[3], const int32_t crlrg[3],
			const float zrg[3] )
{
    auto* p = static_cast<odSeismic3D*>(self);
    if  ( !p || !p->canWrite() )
	return;

    TrcKeyZSampling tkztosave = odSurvey::tkzFromRanges( inlrg, crlrg, zrg,
							 p->zIsTime() ) ;
    p->putData( data, tkztosave );
}




