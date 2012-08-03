#ifndef attribsteering_h
#define attribsteering_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribsteering.h,v 1.4 2012-08-03 13:00:08 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "position.h"
#include "seistype.h"
//#include "commondefs.h"
namespace Attrib
{

mGlobal(AttributeEngine) BinID getSteeringPosition( int );
mGlobal(AttributeEngine) int getSteeringIndex( const BinID& );


}; //Namespace


#endif

