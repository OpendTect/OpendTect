#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "datachar.h"


/*!\brief Info on one component */

mExpClass(General) BasicComponentInfo : public NamedObject
{
public:
			BasicComponentInfo( const char* nm=0 )
			: NamedObject(nm)
			, datatype(0)				{}
			BasicComponentInfo( const BasicComponentInfo& ci )
			: NamedObject((const char*)ci.name())
								{ *this = ci; }
    BasicComponentInfo&	operator=( const BasicComponentInfo& ci )
			{
			    if ( this == &ci ) return *this;
			    setName( ci.name() );
			    datatype = ci.datatype;
			    datachar = ci.datachar;
			    return *this;
			}

    bool		operator==( const BasicComponentInfo& ci ) const
			{
			    return name() == ci.name()
				&& datatype == ci.datatype
				&& datachar == ci.datachar;
			}

    int			datatype;
    DataCharacteristics	datachar;

};
