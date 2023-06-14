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

#include "odsurvey_object.h"

class odSurvey;
class SeisIOObjInfo;

class odSeismicObject : public odSurveyObject
{
public:
    odSeismicObject(const odSurvey& thesurvey, const char* name,
		    const char* tgname);
    odSeismicObject(const odSurvey& thesurvey, const char* name,
		    const BufferStringSet& components, const char* tgname,
		    const char* fmt, bool zistime, bool overwrite);
    ~odSeismicObject();

    odSeismicObject(const odSeismicObject&) = delete;
    odSeismicObject& operator= (const odSeismicObject&) = delete;

    BufferString	getDtypeStr() const;
    BufferString	getDtypeStr(const SeisIOObjInfo&) const;

    int			getNrComponents() const	{ return components_.size(); }
    BufferStringSet*	getCompNames() const;

protected:
    BufferStringSet	components_;
};


