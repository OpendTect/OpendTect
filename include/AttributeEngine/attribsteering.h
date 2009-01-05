#ifndef attribsteering_h
#define attribsteering_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribsteering.h,v 1.2 2009-01-05 09:49:43 cvsranojay Exp $
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
