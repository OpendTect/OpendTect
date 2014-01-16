/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 31/05/04
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "rowcol.h"
#include "bufstring.h"
#include "ptrman.h"
#include "typeset.h"
#include "position.h"
#include "staticstring.h"

#include <math.h>



float RowCol::clockwiseAngleTo(const RowCol& rc) const
{
    const RowCol tmprc(rc);
    const TypeSet<RowCol>& clockwisedirs = RowCol::clockWiseSequence();
    const int selfidx = clockwisedirs.indexOf(*this);
    const float selfangle =  selfidx!=-1 ? selfidx * (float) M_PI_4
			     : atan2( (float)col(), (float)-row() );
    const int rcidx =  clockwisedirs.indexOf(tmprc);
    const float rcangle = rcidx!=-1 ? rcidx * (float) M_PI_4
			     : atan2( (float)tmprc.col(), (float)-tmprc.row() );
    const double twopi = M_2PI;
    float anglediff = rcangle-selfangle;
    if ( anglediff<0 ) anglediff = (float)( anglediff + twopi );
    else if ( anglediff>twopi )
	anglediff = (float)( anglediff - twopi );

    return anglediff;
}


float RowCol::counterClockwiseAngleTo(const RowCol& rc) const
{
    const float twopi = M_2PIf;
    float anglediff = -clockwiseAngleTo(rc);
    if ( anglediff<0 ) anglediff += twopi;
    else if ( anglediff>twopi ) anglediff -= twopi;

    return anglediff;
}


float RowCol::angleTo(const RowCol& rc) const
{
    const float anglediff = clockwiseAngleTo(rc);
    return (float)( anglediff>M_PI ? M_PI*2-anglediff : anglediff );
}


const TypeSet<RowCol>& RowCol::clockWiseSequence()
{
    mDefineStaticLocalObject( PtrMan<TypeSet<RowCol> >, clockwisedirs_, = 0 );

    if ( !clockwisedirs_ )
    {
	TypeSet<RowCol>* newclockwisedirs = new TypeSet<RowCol>;
	(*newclockwisedirs) += RowCol(-1, 0);
	(*newclockwisedirs) += RowCol(-1, 1);

	(*newclockwisedirs) += RowCol( 0, 1);
	(*newclockwisedirs) += RowCol( 1, 1);

	(*newclockwisedirs) += RowCol( 1, 0);
	(*newclockwisedirs) += RowCol( 1,-1);

	(*newclockwisedirs) += RowCol( 0, -1);
	(*newclockwisedirs) += RowCol( -1,-1);

	if ( !clockwisedirs_.setIfNull(newclockwisedirs) )
	    delete newclockwisedirs;
    }

    return *clockwisedirs_;
}


RowCol RowCol::getDirection() const
{
    RowCol res(0,0);
    if ( row()>0 ) res.row()=1;
    else if ( row()<0 ) res.row()=-1;

    if ( col()>0 ) res.col()=1;
    else if ( col()<0 ) res.col()=-1;
    return res;
}





