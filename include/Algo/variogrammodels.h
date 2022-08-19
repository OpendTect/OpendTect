#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include <math.h>


inline void getVariogramModel( const char* typestr, float nugget, float sill,
			      float range, int size, float* in, float* out )
{
    const StringView type( typestr );
    if ( type=="exponential" )
    {
	for ( int idx=0; idx<size; idx++ )
	    out[idx] = (sill-nugget)*(1-exp(-3*in[idx]/range))+nugget;
    }
    else if ( type=="gaussian" )
    {
	for ( int idx=0; idx<size; idx++ )
	{
	    out[idx] = (sill-nugget)*
		    	   (1-exp(-3*((in[idx]*in[idx])/(range*range))))+
			   nugget;
	}
    }
    else if ( type=="spherical" )
    {
	for ( int idx=0; idx<size; idx++ )
	{
	    if ( in[idx] < range )
	    {
		out[idx] = (sill-nugget)*(((3*in[idx])/(2*range))-
			   ((in[idx]*in[idx]*in[idx])/(2*range*range*range)))
		   	   +nugget;
	    }
    	    else
		out[idx] = sill;
	}
    }
}
