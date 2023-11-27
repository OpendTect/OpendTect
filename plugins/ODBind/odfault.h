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

class odFaultObject : public odSurveyObject
{
public:
    odFaultObject(const odSurvey& thesurvey, const char* name, const char* tgname);
    odFaultObject(const odSurvey& thesurvey, const char* name, const char* tgname,
	       bool overwrite );
    ~odFaultObject();

    void	getFeature(OD::JSON::Object&, bool towgs=true) const override;
    void	getPoints(OD::JSON::Array&, bool) const override;

    void	getStick(int stickidx, hAllocator allocator) const;
    int		getNrSticks() const;

protected:
    Geometry::FaultStickSet*	stickset() const;
};


class odFault3D : public odFaultObject
{
public:
    odFault3D(const odSurvey& thesurvey, const char* name);
    odFault3D(const odSurvey& thesurvey, const char* name,
		const StepInterval<int>& inl_rg,
		const StepInterval<int>& crl_rg, bool overwrite=false);
    ~odFault3D();

    void		getInfo(OD::JSON::Object&) const override;

    static const char*	translatorGrp();

protected:

};


class odFaultStickSet : public odFaultObject
{
public:
    odFaultStickSet( const odSurvey& thesurvey, const char* name );
    odFaultStickSet( const odSurvey& thesurvey, const char* name,
		 bool overwrite );
    ~odFaultStickSet();

    void		getInfo(OD::JSON::Object&) const override;

    static const char*	translatorGrp();

protected:

};

typedef void* hSurvey;
mDeclareBaseBindings(Fault3D, fault3d)
mDeclareRemoveBindings(Fault3D, fault3d)

mExternC(ODBind) void fault3d_getstick(hFault3D, int, hAllocator);
mExternC(ODBind) int fault3d_stickcount(hFault3D);

mDeclareBaseBindings(FaultStickSet, faultstickset)
mDeclareRemoveBindings(FaultStickSet, faultstickset)

mExternC(ODBind) void faultstickset_getstick(hFaultStickSet, int, hAllocator);
mExternC(ODBind) int faultstickset_stickcount(hFaultStickSet);

