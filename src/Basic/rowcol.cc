/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 31/05/04
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "rowcol.h"
#include "bufstring.h"
#include "errh.h"
#include "ptrman.h"
#include "typeset.h"
#include "position.h"

#include <math.h>
#include <stdio.h>


mImplRowColFunctions( RowCol, row, col );

float RowCol::clockwiseAngleTo(const RowCol& rc) const
{
    const RowCol tmprc(rc);
    const TypeSet<RowCol>& clockwisedirs = RowCol::clockWiseSequence();
    const int selfidx = clockwisedirs.indexOf(*this);
    const float selfangle =  selfidx!=-1 ? selfidx * (float) M_PI_4 
					 : atan2( (float)col, (float)-row );
    const int rcidx =  clockwisedirs.indexOf(tmprc);
    const float rcangle = rcidx!=-1 ? rcidx * (float) M_PI_4 
				 : atan2( (float)tmprc.col, (float)-tmprc.row );
    static double twopi = M_PI*2;
    float anglediff = rcangle-selfangle;
    if ( anglediff<0 ) anglediff = (float)( anglediff + twopi );
    else if ( anglediff>twopi )
	anglediff = (float)( anglediff - twopi );
    
    return anglediff;
}


float RowCol::counterClockwiseAngleTo(const RowCol& rc) const
{
    static float twopi = (float) M_PI*2;
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


RowCol::RowCol( const BinID& bid )
    : row( bid.inl )
    , col( bid.crl )
{}


const TypeSet<RowCol>& RowCol::clockWiseSequence()
{
    static PtrMan<TypeSet<RowCol> > clockwisedirs_ = new TypeSet<RowCol>;
    if ( !clockwisedirs_->size() )
    {
	(*clockwisedirs_) += RowCol(-1, 0);
	(*clockwisedirs_) += RowCol(-1, 1);

	(*clockwisedirs_) += RowCol( 0, 1);
	(*clockwisedirs_) += RowCol( 1, 1);

	(*clockwisedirs_) += RowCol( 1, 0);
	(*clockwisedirs_) += RowCol( 1,-1);

	(*clockwisedirs_) += RowCol( 0, -1);
	(*clockwisedirs_) += RowCol( -1,-1);
    }

    return *clockwisedirs_;
}


RowCol RowCol::getDirection() const
{
    RowCol res(0,0);
    if ( row>0 ) res.row=1;
    else if ( row<0 ) res.row=-1;

    if ( col>0 ) res.col=1;
    else if ( col<0 ) res.col=-1;
    return res;
}





