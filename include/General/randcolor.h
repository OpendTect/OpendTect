#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "color.h"

namespace OD
{

mGlobal(General) Color getRandomColor(bool withtransp=false);
mGlobal(General) Color getRandStdDrawColor();
mGlobal(General) Color getRandomFillColor();

} // namespace OD
