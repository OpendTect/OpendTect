#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		12-3-2001
 Contents:	Component information
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
			, datatype_(0)				{}
			BasicComponentInfo( const BasicComponentInfo& ci )
			: NamedObject((const char*)ci.name())
								{ *this = ci; }
    BasicComponentInfo&	operator=( const BasicComponentInfo& ci )
			{
			    if ( this == &ci ) return *this;
			    setName( ci.name() );
			    datatype_ = ci.datatype_;
			    datachar_ = ci.datachar_;
			    return *this;
			}

    bool		operator==( const BasicComponentInfo& ci ) const
			{
			    return hasName( ci.name() )
				&& datatype_ == ci.datatype_
				&& datachar_ == ci.datachar_;
			}

    int			datatype_;
    DataCharacteristics	datachar_;

};
