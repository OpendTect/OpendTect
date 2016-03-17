/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 31/05/04
-*/


#include "rowcol.h"
#include "bufstring.h"
#include "ptrman.h"
#include "typeset.h"
#include "position.h"
#include "perthreadrepos.h"

#include <math.h>



float RowCol::clockwiseAngleTo(const RowCol& rc) const
{
    const RowCol tmprc(rc);
    const TypeSet<RowCol>& clockwisedirs = RowCol::clockWiseSequence();
    const int selfidx = clockwisedirs.indexOf(*this);
    const float selfangle =  selfidx!=-1 ? selfidx * M_PI_4f
			  : Math::Atan2( (float)col(), (float)-row() );
    const int rcidx =  clockwisedirs.indexOf(tmprc);
    const float rcangle = rcidx!=-1 ? rcidx * M_PI_4f
			: Math::Atan2( (float)tmprc.col(),(float)-tmprc.row() );
    float anglediff = rcangle - selfangle;
    if ( anglediff < 0 )
	anglediff += M_2PIf;
    else if ( anglediff > M_2PIf )
	anglediff -= M_2PIf;

    return anglediff;
}


float RowCol::counterClockwiseAngleTo(const RowCol& rc) const
{
    float anglediff = -clockwiseAngleTo(rc);
    if ( anglediff < 0 )
	anglediff += M_2PIf;
    else if ( anglediff > M_2PIf )
	anglediff -= M_2PIf;

    return anglediff;
}


float RowCol::angleTo(const RowCol& rc) const
{
    const float anglediff = clockwiseAngleTo(rc);
    return anglediff > M_PIf ? M_2PIf-anglediff : anglediff;
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

	clockwisedirs_.setIfNull(newclockwisedirs,true);
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
