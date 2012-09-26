/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
    sort_array( d2tarr, d2tsz );
    sort_array( daharr, d2tsz );

    const Interval<float>& dahrg = d2t.dahRange();
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	if ( !dahrg.includes( cs.dah( idx ), true ) )
	    d2t.insertAtDah( cs.dah(idx), cs.value( idx ) );
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
    IdxAble::callibrateArray( d2tarr, d2tsz,
			      csarr, ctrlsamples.arr(), 
			      cs.size(), false, d2tarr );
}

}; //namespace WellTie
