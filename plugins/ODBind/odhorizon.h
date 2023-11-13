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

#include <tuple>

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
		const StepInterval<int>& inl_rg,
		const StepInterval<int>& crl_rg, bool overwrite=false);
    ~odHorizon3D();

    void		getZ(hAllocator);
    void		getXY(hAllocator);
    void		putZ(const uint32_t shape[2], const float* data,
			     const int32_t* inlines, const int32_t* crlines);
    void		putZ(const uint32_t shape[2], const float* data,
			     const double* xpos, const double* ypos);

    void		getInfo(OD::JSON::Object&) const override;
    void		getPoints(OD::JSON::Array&, bool) const override;

    static const char*	translatorGrp();

protected:
    TrcKeySampling		tk_;
    PtrMan<Array2D<float>>	array_;
    size_t			writecount_ = 0;

    void			save();

};


class odHorizon2D : public odEMObject
{
public:
    odHorizon2D( const odSurvey& thesurvey, const char* name );
    odHorizon2D( const odSurvey& thesurvey, const char* name, bool creategeom,
		 bool overwrite=false );
    ~odHorizon2D();

    BufferStringSet*	getLineNames() const;
    BufferString	getLineName(int lineid) const;
    int			getNrLines() const;
    void		getLineIDs( int num, int* ids) const;
    void		getZ(hAllocator, int lineid);
    void		getXY(hAllocator, int lineid);

    void		getInfo(OD::JSON::Object&) const override;
    void		getFeature(OD::JSON::Object&,
				   bool towgs=true) const override;
    void		getPoints(OD::JSON::Array&, bool) const override;

    static const char*	translatorGrp()		{ return "2D Horizon"; }

protected:
    RefMan<EM::Horizon2D>	hor_;
    bool			creategeom_;

};

typedef void* hSurvey;
mDeclareBaseBindings(Horizon3D, horizon3d)
mDeclareRemoveBindings(Horizon3D, horizon3d)

mExternC(ODBind) hHorizon3D	horizon3d_newout(hSurvey, const char* name,
						 const int* inl_rg,
						 const int* crl_rg,
						 bool overwrite);
mExternC(ODBind) hStringSet	horizon3d_attribnames(hHorizon3D);
mExternC(ODBind) void		horizon3d_getz(hHorizon3D, hAllocator);
mExternC(ODBind) void		horizon3d_getxy(hHorizon3D, hAllocator);
mExternC(ODBind) void		horizon3d_putz(hHorizon3D,
					       const uint32_t shape[2],
					       const float* data,
					       const int32_t* inlines,
					       const int32_t* crlines);
mExternC(ODBind) void		horizon3d_putz_byxy(hHorizon3D,
						    const uint32_t shape[2],
						    const float* data,
						    const double* xpos,
						    const double* ypos);


mDeclareBaseBindings(Horizon2D, horizon2d)
mDeclareRemoveBindings(Horizon2D, horizon2d)

mExternC(ODBind) hHorizon2D	horizon2d_newout(hSurvey, const char* name,
						 bool creategeom,
						 bool overwrite);
mExternC(ODBind) hStringSet	horizon2d_attribnames(hHorizon2D);
mExternC(ODBind) int		horizon2d_linecount(hHorizon2D);
mExternC(ODBind) void		horizon2d_lineids(hHorizon2D, int, int*);
mExternC(ODBind) const char*	horizon2d_linename(hHorizon2D, int);
mExternC(ODBind) hStringSet	horizon2d_linenames(hHorizon2D);
mExternC(ODBind) void		horizon2d_getz(hHorizon2D, hAllocator, int);
mExternC(ODBind) void		horizon2d_getxy(hHorizon2D, hAllocator, int);
