/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    const int d2tsz = d2t .size();
    if ( d2tsz < 2 ) return;

    float* d2tarr = d2t.valArr();
    float* daharr = d2t.dahArr();
    bool found = false;

    sort_array( d2tarr, d2tsz );
    sort_array( daharr, d2tsz );
    
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	if ( mIsUdf(cs.dah(idx)) ) continue;
	found = false;
	for ( int insertidx=0; insertidx<d2tsz; insertidx++ )
	{
	    if ( mIsEqual(cs.dah(idx),d2t.dah(insertidx),1e-3) )
	    {
		found = true;
		break;
	    }

	    if ( d2t.dah(insertidx) > cs.dah(idx) ) 
		break;
	}

	if ( !found )
	    d2t.insertAtDah( cs.dah(idx), cs.value(idx) );
    }

    TypeSet<int> ctrlsamples;
    d2tarr = d2t.valArr();
    daharr = d2t.dahArr();
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	const int dahidx = d2t.indexOf( cs.dah(idx) );
	ctrlsamples += dahidx;
    }
    const float* csarr = cs.valArr();
    IdxAble::calibrateArray( d2tarr, d2tsz,
			      csarr, ctrlsamples.arr(),
			      cs.size(), false, d2tarr );
}

} // namespace WellTie
