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
#include "seis2ddata.h"
#include "odseismic_object.h"

class odSurvey;


class odSeismic2D : public odSeismicObject
{
public:
    enum Seis2DFormat { CBVS, SEGYDirect };
    mDeclareEnumUtils(Seis2DFormat);

    odSeismic2D(const odSurvey& thesurvey, const char* name);
    odSeismic2D(const odSurvey& thesurvey, const char* name, Seis2DFormat fmt,
		const BufferStringSet& components, bool zistime,
		bool overwrite);
    ~odSeismic2D();

    odSeismic2D(const odSeismic2D&) = delete;
    odSeismic2D& operator= (const odSeismic2D&) = delete;

    void		close();
    int			getNrLines() const;
    BufferStringSet*	getLineNames() const;
    void		getLineInfo(OD::JSON::Array&,
				    const BufferStringSet&) const;
    void		getData(hAllocator, const char* line,
				float zrg[3]) const;
    void		putData(const char* line, const float** data,
				int32_t ntrcs, int32_t nrz,
				const float zrg[3], const int32_t* trcnrs);
    bool		delLines(const BufferStringSet&);
    void		getInfo(OD::JSON::Object&) const override;
    void		getFeature(OD::JSON::Object&,
				   bool towgs=true) const override;
    void		getPoints(OD::JSON::Array&, bool towgs) const override;

    static const char*	translatorGrp()		{return "2D Seismic Data";}

protected:
    PtrMan<Seis2DDataSet>	seisdata_;
};

mDeclareBaseBindings(Seismic2D, seismic2d)
mDeclareRemoveBindings(Seismic2D, seismic2d)

mExternC(ODBind) hSeismic2D	seismic2d_newout(hSurvey, const char* name,
						 const char* format,
						 hStringSet compnames,
						 bool zistime,
						 bool overwrite);
mExternC(ODBind) void		seismic2d_close(hSeismic2D);
mExternC(ODBind) hStringSet	seismic2d_compnames(hSeismic2D);
mExternC(ODBind) hStringSet	seismic2d_linenames(hSeismic2D);
mExternC(ODBind) const char*	seismic2d_lineinfo(hSeismic2D,
						   const hStringSet);
mExternC(ODBind) void		seismic2d_getdata(hSeismic2D, hAllocator,
						  const char* linenm,
						  float zrg[3]);
mExternC(ODBind) void		seismic2d_putdata(hSeismic2D, const char*,
						  const float** data,
						  int32_t ntrcs,
						  int32_t nrz,
						  const float zrg[3],
						  const int32_t* trcnrs);
mExternC(ODBind) bool		seismic2d_deletelines(hSeismic2D,
						      const hStringSet);






