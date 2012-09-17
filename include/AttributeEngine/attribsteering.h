#ifndef attribsteering_h
#define attribsteering_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribsteering.h,v 1.3 2009/07/22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "seistype.h"
//#include "commondefs.h"
namespace Attrib
{

mGlobal BinID getSteeringPosition( int );
mGlobal int getSteeringIndex( const BinID& );


}; //Namespace


#endif
