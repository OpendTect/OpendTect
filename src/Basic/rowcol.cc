/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 31/05/04
-*/

static const char* rcsID = "$Id: rowcol.cc,v 1.5 2004-06-17 07:48:07 kristofer Exp $";

#include "rowcol.h"
#include "ptrman.h"
#include "sets.h"
#include <math.h>

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


float RowCol::clockwiseAngleTo(const RowCol& rc) const
{
    const TypeSet<RowCol> clockwisedirs = clockWiseSequence();
    const int selfidx = clockwisedirs.indexOf(*this);
    const float selfangle = selfidx!=-1 ? selfidx * M_PI_4 : atan2( col, -row );
    const int rcidx =  clockwisedirs.indexOf(rc);
    const float rcangle = rcidx!=-1 ? rcidx * M_PI_4 : atan2( rc.col, -rc.row );

    static double twopi = M_PI*2;
    float anglediff = rcangle-selfangle;
    if ( anglediff<0 ) anglediff+=twopi;
    else if ( anglediff>twopi ) anglediff-=twopi;

    return anglediff;
}


float RowCol::counterClockwiseAngleTo(const RowCol& rc) const
{
    static double twopi = M_PI*2;
    float anglediff = -clockwiseAngleTo(rc);
    if ( anglediff<0 ) anglediff+=twopi;
    else if ( anglediff>twopi ) anglediff-=twopi;

    return anglediff;
}


float RowCol::angleTo(const RowCol& rc) const
{
    const float anglediff = clockwiseAngleTo(rc);
    return anglediff>M_PI_2 ? M_PI-anglediff : anglediff;
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


bool RowCol::isNeighborTo( const RowCol& rc, const RowCol& step,
			   bool eightconnectivity ) const
{
    const RowCol diff(abs(row-rc.row),abs(col-rc.col));
    bool areeightconnected = diff.row<=step.row && diff.col<=step.col;
    if ( eightconnectivity )
	return areeightconnected;

    return areeightconnected && (diff.row>0+diff.col>0)<2;
}

