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
#include "odseismic_object.h"

#include "seisioobjinfo.h"


odSeismicObject::odSeismicObject( const odSurvey& thesurvey, const char* name,
				  const char* tgname )
    : odSurveyObject(thesurvey, name, tgname)
{
}


odSeismicObject::odSeismicObject( const odSurvey& thesurvey, const char* name,
				  const char* tgname, bool overwrite,
				  const char* fmt )
    : odSurveyObject(thesurvey, name, tgname, overwrite, fmt)
{
}


odSeismicObject::~odSeismicObject()
{}


int odSeismicObject::getNrComponents() const
{
    survey_.activate();
    SeisIOObjInfo seisinfo( ioobj_ );
    return seisinfo.nrComponents();
}


BufferString odSeismicObject::getDtypeStr() const
{
    survey_.activate();
    DataCharacteristics dc;
    SeisIOObjInfo seisinfo( ioobj_ );
    seisinfo.getDataChar( dc );
    BufferString res;
    dc.toString( res );
    return res;
}


BufferStringSet* odSeismicObject::getCompNames( int lineid ) const
{
    survey_.activate();
    BufferStringSet* nms = new BufferStringSet;
    SeisIOObjInfo seisinfo( ioobj_ );
    if( lineid==-1 )
	seisinfo.getComponentNames( *nms );
    else
	seisinfo.getComponentNames( *nms, Pos::GeomID(lineid) );

    return nms;
}


