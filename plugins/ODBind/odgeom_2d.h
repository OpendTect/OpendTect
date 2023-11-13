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
#include "odsurvey_object.h"

class odSurvey;


class odGeom2D : public odSurveyObject
{
public:
    odGeom2D(const odSurvey& thesurvey, const char* name);
    odGeom2D(const odSurvey& thesurvey, const char* name, bool overwrite);
    ~odGeom2D();

    odGeom2D(const odGeom2D&) = delete;
    odGeom2D& operator= (const odGeom2D&) = delete;

    void		close();
    void		getData(hAllocator) const;
    void		putData( int32_t numpos, const int32_t* trcnrs,
				 const float* spnrs,
				 const double* xpos, const double* ypos);

    void		getInfo(OD::JSON::Object&) const override;
    void		getFeature(OD::JSON::Object&,
				   bool towgs=true) const override;
    void		getPoints(OD::JSON::Array&, bool towgs) const override;

    static const char*	translatorGrp()		{return "Geometry";}

protected:
};

mDeclareBaseBindings(Geom2D, geom2d)
mExternC(ODBind) hGeom2D	geom2d_newout(hSurvey, const char* name,
					      bool overwrite);
mExternC(ODBind) void		geom2d_close(hGeom2D);
mExternC(ODBind) void		geom2d_get(hGeom2D, hAllocator);
mExternC(ODBind) void		geom2d_put(hGeom2D, int32_t numpos,
					   const int32_t* trcnrs,
					   const float* spnrs,
					   const double* xpos,
					   const double* ypos);
mExternC(ODBind) void		geom2d_removeobjs(hSurvey, const hStringSet);




