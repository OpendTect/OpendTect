#ifndef basiccompinfo_h
#define basiccompinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Component information
 RCS:		$Id: basiccompinfo.h,v 1.11 2012-08-03 13:00:21 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "datachar.h"


/*!\brief Info on one component */

mClass(General) BasicComponentInfo : public NamedObject
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


#endif

