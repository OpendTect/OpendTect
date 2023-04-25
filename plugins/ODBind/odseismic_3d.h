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

#include "ptrman.h"
#include "trckeyzsampling.h"
#include "odseismic_object.h"


class odSurvey;
class SeisTrcWriter;
class SeisSequentialWriter;


class odSeismic3D : public odSeismicObject
{
public:
    enum Seis3DFormat { CBVS, SEGYDirect };
    mDeclareEnumUtils(Seis3DFormat);

    odSeismic3D(const odSurvey& thesurvey, const char* name);
    odSeismic3D(const odSurvey& thesurvey, const char* name, Seis3DFormat fmt,
		bool overwrite=false);
    odSeismic3D(const odSeismic3D&);
    ~odSeismic3D();

    int			getNrTraces() const;

    void		getInfo(OD::JSON::Object&) const override;
    void		getPoints(OD::JSON::Array&, bool towgs) const override;

    const TrcKeyZSampling&	tkz() const	{ return tkz_; }

    static const char*	sKeyTranslatorGrp()	{ return "Seismic Data"; }

protected:
    TrcKeyZSampling		tkz_;
    StepInterval<float>		zputrg_;
    SeisTrcWriter*		writer_ = nullptr;
    size_t			writecount_ = 0;
    SeisSequentialWriter*	sequentialwriter_ = nullptr;

};

mDeclareBaseBindings(Seismic3D, seismic3d)
mExternC(ODBind) hStringSet	seismic3d_compnames(hSeismic3D);

mExternC(ODBind) void		seismic3d_getinlcrl(hSeismic3D, size_t, int*,
						    int*);
mExternC(ODBind) od_int64	seismic3d_gettrcidx(hSeismic3D, int, int);
mExternC(ODBind) od_int64	seismic3d_nrbins(hSeismic3D);
mExternC(ODBind) od_int64	seismic3d_nrtrcs(hSeismic3D);




