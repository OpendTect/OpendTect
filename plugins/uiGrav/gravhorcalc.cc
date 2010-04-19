/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: gravhorcalc.cc,v 1.1 2010-04-19 15:14:27 cvsbert Exp $";

#include "gravhorcalc.h"


Grav::HorCalc::HorCalc( const MultiID& calc, const MultiID* top,
			const MultiID* bot, float ang )
    : Executor("Calculate gravity on horizon")
    , cutoffangle_(ang)
{
}
