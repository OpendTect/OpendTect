/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 31/05/04
-*/

static const char* rcsID = "$Id: rowcol.cc,v 1.10 2004-09-21 15:58:49 kristofer Exp $";

#include "rowcol.h"

#include "errh.h"
#include "ptrman.h"
#include "sets.h"
#include <math.h>

bool RCol::isNeighborTo( const RCol& rc, const RCol& step,
			 bool eightconnectivity ) const
{
    const RowCol diff(abs(r()-rc.r()),abs(c()-rc.c()));
    bool areeightconnected = diff.row<=step.r() && diff.col<=step.c() &&
			     !(!diff.row && !diff.col);
    if ( eightconnectivity )
	return areeightconnected;

    return areeightconnected && (diff.row>0+diff.col>0)<2;
}


float RCol::clockwiseAngleTo(const RCol& rc_) const
{
    const RowCol rc(rc_);
    const TypeSet<RowCol> clockwisedirs = RowCol::clockWiseSequence();
    const int selfidx = clockwisedirs.indexOf(*this);
    const float selfangle = selfidx!=-1 ? selfidx * M_PI_4 : atan2( c(), -r() );
    const int rcidx =  clockwisedirs.indexOf(rc);
    const float rcangle = rcidx!=-1 ? rcidx * M_PI_4 : atan2( rc.col, -rc.row );

    static double twopi = M_PI*2;
    float anglediff = rcangle-selfangle;
    if ( anglediff<0 ) anglediff+=twopi;
    else if ( anglediff>twopi ) anglediff-=twopi;

    return anglediff;
}


float RCol::counterClockwiseAngleTo(const RCol& rc) const
{
    static double twopi = M_PI*2;
    float anglediff = -clockwiseAngleTo(rc);
    if ( anglediff<0 ) anglediff+=twopi;
    else if ( anglediff>twopi ) anglediff-=twopi;

    return anglediff;
}


float RCol::angleTo(const RCol& rc) const
{
    const float anglediff = clockwiseAngleTo(rc);
    return anglediff>M_PI ? M_PI*2-anglediff : anglediff;
}


int RCol::distanceSq( const RCol& rc ) const
{
    int rdiff = r()-rc.r();
    int cdiff = c()-rc.c();
    return rdiff*rdiff+cdiff*cdiff;
}


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





