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

#include "emhorizon2d.h"
#include "odjson.h"
#include "ptrman.h"
#include "trckeysampling.h"

#include "odbind.h"
#include "odsurvey_object.h"

class odSurvey;
template <class T>
class Array2D;

class odEMObject : public odSurveyObject
{
public:
    odEMObject(const odSurvey& thesurvey, const char* name, const char* tgname);
    odEMObject(const odSurvey& thesurvey, const char* name, const char* tgname,
	       bool overwrite );
    ~odEMObject();

    virtual BufferStringSet*	getAttribNames() const;
    virtual int			getNrAttributes() const;
};


class odHorizon3D : public odEMObject
{
public:
    odHorizon3D(const odSurvey& thesurvey, const char* name);
    odHorizon3D(const odSurvey& thesurvey, const char* name,
		const StepInterval<int>& ilines,
		const StepInterval<int>& xlines, bool overwrite=false);
    ~odHorizon3D();

    StepInterval<int>	ilines() const;
    StepInterval<int>	xlines() const;
    void		close();
    void		getData();
//    void		putData(const py::object&, bool bycoord=false);
//    py::dict		getCoords(const py::list&) const;

    void		getInfo(OD::JSON::Object&) const override;
    void		getPoints(OD::JSON::Array&, bool) const override;

    static const char*	sKeyTranslatorGrp();

protected:
    TrcKeySampling		tk_;
    PtrMan<Array2D<float>>	array_;
    size_t			writecount_ = 0;
};


class odHorizon2D : public odEMObject
{
public:
    odHorizon2D( const odSurvey& thesurvey, const char* name );
    odHorizon2D( const odSurvey& thesurvey, const char* name, bool creategeom,
		 bool overwrite=false );
    ~odHorizon2D();

    BufferStringSet*	getLineNames() const;
    int			getNrLines() const;
    void		getLineIDs( int num, int* ids) const;
    void		close();
    // py::object		getData(int) const;
    // py::dict		getData(const py::tuple& tup) const;
    void		getInfo(OD::JSON::Object&) const override;
    void		getPoints(OD::JSON::Array&, bool) const override;

    static const char*	sKeyTranslatorGrp()	{ return "2D Horizon"; }

protected:
    // RefMan<EM::Horizon2D>	hor_;
    bool			creategeom_;

};

typedef void* hSurvey;
typedef void* hHorizon2D;
typedef void* hHorizon3D;

mExternC(ODBind) hHorizon3D	horizon3d_newin(hSurvey, const char* name);
mExternC(ODBind) hHorizon3D	horizon3d_newout(hSurvey, const char* name,
						 const intStepInterval iline,
						 const intStepInterval xline,
						 bool overwrite);
mExternC(ODBind) void		horizon3d_del(hHorizon3D);
mExternC(ODBind) int		horizon3d_attribcount(hHorizon3D);
mExternC(ODBind) hStringSet	horizon3d_attribnames(hHorizon3D);
mExternC(ODBind) const char*	horizon3d_feature(hHorizon3D);
mExternC(ODBind) const char*	horizon3d_features(hSurvey, const hStringSet);
mExternC(ODBind) const char*	horizon3d_info(hHorizon3D);
mExternC(ODBind) const char*	horizon3d_infos(hSurvey, const hStringSet);
mExternC(ODBind) const char*	horizon3d_name(hHorizon3D);
mExternC(ODBind) hStringSet	horizon3d_names(hSurvey);

mExternC(ODBind) hHorizon2D	horizon2d_new(hSurvey, const char* name);
mExternC(ODBind) void		horizon2d_del(hHorizon2D);

mExternC(ODBind) int		horizon2d_attribcount(hHorizon2D);
mExternC(ODBind) hStringSet	horizon2d_attribnames(hHorizon2D);
mExternC(ODBind) const char*	horizn2d_errmsg(hHorizon2D);
mExternC(ODBind) const char*	horizn2d_feature(hHorizon2D);
mExternC(ODBind) const char*	horizon2d_info(hHorizon2D);
mExternC(ODBind) const char*	horizon2d_infos(hSurvey, const hStringSet);
mExternC(ODBind) bool		horizon2d_isok(hHorizon2D);
mExternC(ODBind) int		horizon2d_linecount(hHorizon2D);
mExternC(ODBind) void		horizon2d_lineids(hHorizon2D, int, int*);
mExternC(ODBind) hStringSet	horizon2d_linenames(hHorizon2D);
mExternC(ODBind) const char*	horizon2d_name(hHorizon2D);
mExternC(ODBind) hStringSet	horizon2d_names(hSurvey);
