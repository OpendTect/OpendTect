#ifndef basiccompinfo_h
#define basiccompinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Component information
 RCS:		$Id: basiccompinfo.h,v 1.7 2004-07-16 15:35:25 bert Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "datachar.h"


/*!\brief Info on one component */

class BasicComponentInfo : public UserIDObject
{
public:
			BasicComponentInfo( const char* nm=0 )
			: UserIDObject(nm)
			, datatype(0)				{}
			BasicComponentInfo( const BasicComponentInfo& ci )
			: UserIDObject((const char*)ci.name())
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
