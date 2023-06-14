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
    const SeisIOObjInfo seisinfo( ioobj_ );
    zistime_ = seisinfo.isTime();
    seisinfo.getComponentNames( components_ );
}


odSeismicObject::odSeismicObject( const odSurvey& thesurvey, const char* name,
				  const BufferStringSet& components,
				  const char* tgname, const char* fmt,
				  bool zistime, bool overwrite )
    : odSurveyObject(thesurvey, name, tgname, overwrite, fmt)
{
    zistime_ = zistime;
    components_.add( components, true );
}


odSeismicObject::~odSeismicObject()
{}


BufferString odSeismicObject::getDtypeStr() const
{
    survey_.activate();
    PtrMan<SeisIOObjInfo> info = new SeisIOObjInfo(ioobj_);
    return getDtypeStr( *info );
}


BufferString odSeismicObject::getDtypeStr( const SeisIOObjInfo& info ) const
{
    DataCharacteristics dc;
    info.getDataChar( dc );
    BufferString res;
    dc.toString( res );
    return res;
}


BufferStringSet* odSeismicObject::getCompNames() const
{
    auto* nms = new BufferStringSet;
    nms->add( components_, true );
    return nms;
}



