#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
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
