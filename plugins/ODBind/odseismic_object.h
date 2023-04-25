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

class odSeismicObject : public odSurveyObject
{
public:
    odSeismicObject(const odSurvey& thesurvey, const char* name,
		    const char* tgname);
    odSeismicObject(const odSurvey& thesurvey, const char* name,
		    const char* tgname, bool overwrite, const char* fmt);
    ~odSeismicObject();

    BufferStringSet*	getCompNames(int lineid=-1) const;
    BufferString	getDtypeStr() const;
    int			getNrComponents() const;

protected:

};


