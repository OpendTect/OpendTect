#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "binid.h"
#include "seistype.h"
//#include "commondefs.h"
namespace Attrib
{

mGlobal(AttributeEngine) BinID getSteeringPosition( int );
mGlobal(AttributeEngine) int getSteeringIndex( const BinID& );


} // namespace Attrib
