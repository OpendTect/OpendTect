#ifndef attribsteering_h
#define attribsteering_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribsteering.h,v 1.1 2005-02-01 14:05:34 kristofer Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "seistype.h"

namespace Attrib
{

BinID getSteeringPosition( int );
int getSteeringIndex( const BinID& );


}; //Namespace


#endif
