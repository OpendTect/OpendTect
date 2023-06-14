#pragma once
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
#include "odbindmod.h"
#include "odbind.h"

#include "ptrman.h"
#include "trckeyzsampling.h"
#include "odseismic_object.h"


class odSurvey;
class SeisTrcWriter;
class SeisSequentialWriter;
namespace PosInfo{ class CubeDataIndex; }


class odSeismic3D : public odSeismicObject
{
public:
    enum Seis3DFormat { CBVS, SEGYDirect };
    mDeclareEnumUtils(Seis3DFormat);

    odSeismic3D(const odSurvey& thesurvey, const char* name);
    odSeismic3D(const odSurvey& thesurvey, const char* name, Seis3DFormat fmt,
		const BufferStringSet& components,
		const TrcKeyZSampling& tkz, bool zistime, bool overwrite);
    ~odSeismic3D();

    odSeismic3D(const odSeismic3D&) = delete;
    odSeismic3D& operator= (const odSeismic3D&) = delete;

    void		close();
    od_int64		getNrTraces() const;
    od_int64		getTrcNum(const BinID&) const;
    BinID		getBinID(od_int64) const;
    StepInterval<float>	getZrange() const;
    StepInterval<float>	getZrange(const SeisIOObjInfo&) const;
    void		getData(hAllocator, const TrcKeyZSampling&) const;
    void		putData(const float** data, const TrcKeyZSampling&);

    void		getInfo(OD::JSON::Object&) const override;
    void		getPoints(OD::JSON::Array&, bool towgs) const override;

    const TrcKeyZSampling&	tkz() const	{ return tkz_; }

    static const char*	translatorGrp()		{return "Seismic Data";}

protected:
    PtrMan<PosInfo::CubeDataIndex>	cubeidx_;
    PtrMan<SeisTrcWriter>		writer_;
    PtrMan<SeisSequentialWriter>	sequentialwriter_;
    TrcKeyZSampling			tkz_;
    size_t				writecount_ = 0;

    int					makeDims(const TrcKeyZSampling&,
						 TypeSet<int>& dims) const;

};

mDeclareBaseBindings(Seismic3D, seismic3d)
mExternC(ODBind) hSeismic3D	seismic3d_newout(hSurvey, const char* name,
						 const char* format,
						 hStringSet compnames,
						 const int32_t inlrg[3],
						 const int32_t crlrg[3],
						 const float zrg[3],
						 bool zistime,
						 bool overwrite);
mExternC(ODBind) void		seismic3d_close(hSeismic3D);
mExternC(ODBind) hStringSet	seismic3d_compnames(hSeismic3D);
mExternC(ODBind) int		seismic3d_getzidx(hSeismic3D, float);
mExternC(ODBind) float		seismic3d_getzval(hSeismic3D, int32_t);
mExternC(ODBind) void		seismic3d_getinlcrl(hSeismic3D, size_t,
						    int32_t*, int32_t*);
mExternC(ODBind) od_int64	seismic3d_gettrcidx(hSeismic3D, int32_t,
						    int32_t);
mExternC(ODBind) od_int64	seismic3d_nrbins(hSeismic3D);
mExternC(ODBind) od_int64	seismic3d_nrtrcs(hSeismic3D);
mExternC(ODBind) void		seismic3d_zrange(hSeismic3D, float zrg[3]);
mExternC(ODBind) bool		seismic3d_validrange(hSeismic3D,
						     const int32_t inlrg[3],
						     const int32_t crlrg[3],
						     const float zrg[3]);
mExternC(ODBind) void		seismic3d_getdata(hSeismic3D, hAllocator,
						  const int32_t inlrg[3],
						  const int32_t crlrg[3],
						  const float zrg[3]);
mExternC(ODBind) void		seismic3d_putdata(hSeismic3D,
						  const float** data,
						  const int32_t inlrg[3],
						  const int32_t crlrg[3],
						  const float zrg[3]);





