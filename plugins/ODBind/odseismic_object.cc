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
    along with this program. If not, see <https://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/
#include "odseismic_object.h"

#include "ioman.h"
#include "seisioobjinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"


odSeismicObject::odSeismicObject( const odSurvey& thesurvey, const char* name,
				  const char* tgname )
    : odSurveyObject(thesurvey, name, tgname)
{
    ConstPtrMan<IOObj> ioobj = ioobj_ptr();
    if ( ioobj )
    {
	const SeisIOObjInfo seisinfo( ioobj.ptr() );
	zistime_ = seisinfo.isTime();
	seisinfo.getComponentNames( components_ );
    }
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
    errmsg_.setEmpty();
    BufferString res;
    ConstPtrMan<IOObj> ioobj = ioobj_ptr();
    if ( ioobj )
    {
	PtrMan<SeisIOObjInfo> info = new SeisIOObjInfo( ioobj.ptr() );
	res = getDtypeStr( *info );
    }

    return res;
}


BufferString odSeismicObject::getDtypeStr( const SeisIOObjInfo& info ) const
{
    errmsg_.setEmpty();
    DataCharacteristics dc;
    info.getDataChar( dc );
    BufferString res;
    dc.toString( res );
    return res;
}


BufferStringSet* odSeismicObject::getCompNames() const
{
    errmsg_.setEmpty();
    auto* nms = new BufferStringSet;
    nms->add( components_, true );
    return nms;
}


void odSeismicObject::setVelocityType( OD::VelocityType type,
				       const char* velunit )
{
    errmsg_.setEmpty();
    PtrMan<IOObj> ioobj( ioobj_ptr() );
    if ( !ioobj )
	return;

    VelocityDesc desc( type );
    const bool hasunit = velunit && *velunit;
    if ( desc.isVelocity() )
    {
	const UnitOfMeasure* uom = hasunit ? UoMR().get( velunit )
				: UnitOfMeasure::surveyDefVelUnit();
	if ( !uom || uom->propType()!=Mnemonic::Vel )
	{
	    errmsg_ = "invalid velocity unit";
	    return;
	}

	desc.setUnit( uom );
    }
    else if ( hasunit )
    {
	errmsg_ = "invalid velocity unit";
	return;
    }

    if ( desc.isUdf() )
	VelocityDesc::removePars( ioobj->pars() );
    else
	desc.fillPar( ioobj->pars() );

    if ( !IOM().commitChanges(*ioobj) )
	errmsg_ = "cannot write the velocity type";
}


void odSeismicObject::getVelocityInfo( OD::JSON::Object& jsobj,
				       const IOObj& ioobj ) const
{
    VelocityDesc veldesc;
    if ( !veldesc.usePar(ioobj.pars()) || veldesc.isUdf() )
	return;

    jsobj.set( "velocity_type", OD::toString(veldesc.type_) );
    if ( veldesc.hasVelocityUnit() )
	jsobj.set( "velocity_unit", veldesc.getUnit()->getLabel() );
}
