#ifndef staticsdesc_h
#define staticsdesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		May 2009
 RCS:		$Id: staticsdesc.h,v 1.2 2009-05-14 18:32:03 cvskris Exp $
________________________________________________________________________

*/

#include "multiid.h"

class IOPar;

/*!Specifies Statics as a horizon and either a horizon attribute or 
   a constant velocity. Velocity is always in m/s. */

mClass StaticsDesc
{
public:
			StaticsDesc();

    MultiID		horizon_;
    float		vel_;
    BufferString	velattrib_;	//attrib on statichorizon_
    					//if empty, use vel

    bool		operator==(const StaticsDesc&) const;
    bool		operator!=(const StaticsDesc&) const;

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyHorizon();
    static const char*	sKeyVelocity();
    static const char*	sKeyVelocityAttrib();
};


#endif
