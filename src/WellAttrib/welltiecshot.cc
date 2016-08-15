/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/

#include "welltiecshot.h"

#include "idxable.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

void CheckShotCorr::calibrate( const Well::D2TModel& cs, Well::D2TModel& d2t )
{
    if ( cs.isEmpty() || d2t.isEmpty() )
	return;

    d2t.calibrateBy( cs );

    Well::D2TModelIter iter( cs );
    while ( iter.next() )
	d2t.setValueAt( iter.dah(), iter.value() );
}

} // namespace WellTie
