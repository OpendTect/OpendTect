#ifndef variogrammodels_h
#define variogrammodels_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck & H.Huck
 Date:		27-09-2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include <math.h>

template <class T> class TypeSet;
template <class T> class ObjectSet;

inline void getVariogramModel( const char* typestr, float nugget, float sill,
			      float range, int size, float* in, float* out )
{
    if ( !strcmp( typestr, "exponential" ) )
	for ( int idx=0; idx<size; idx++ )
	    out[idx] = (sill-nugget)*(1-exp(-3*in[idx]/range))+nugget;
    else if ( !strcmp( typestr, "gaussian" ) )
	for ( int idx=0; idx<size; idx++ )
	    out[idx] = (sill-nugget)*
		    	   (1-exp(-3*((in[idx]*in[idx])/(range*range))))+
			   nugget;
    else if ( !strcmp( typestr, "spherical" ) )
	for ( int idx=0; idx<size; idx++ )
	    if ( in[idx] < range )
	    {
		out[idx] = (sill-nugget)*(((3*in[idx])/(2*range))-
			   ((in[idx]*in[idx]*in[idx])/(2*range*range*range)))
		   	   +nugget;
	    }
    	    else
		out[idx] = sill;
}


#endif
