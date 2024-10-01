/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "rowcol.h"

#include "math2.h"
#include "ptrman.h"
#include "typeset.h"

#include <math.h>


RowCol::RowCol()
{}


RowCol::RowCol( int i, int c )
    : Pos::IdxPair(i,c)
{
}


RowCol::RowCol( const RowCol& rc )
    : Pos::IdxPair(rc.row(),rc.col())
{}


RowCol::RowCol( Pos::IdxPair p )
    : Pos::IdxPair(p)
{
}


RowCol::~RowCol()
{}


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
