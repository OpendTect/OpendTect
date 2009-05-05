#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.9 2009-05-05 21:00:00 cvskris Exp $
________________________________________________________________________

*/

#include "enums.h"
#include "multiid.h"
#include "staticsdesc.h"


/*!Specifies velocity type and statics for a velocity.   */


mClass VelocityDesc
{
public:
    enum Type		{ Unknown, Interval, RMS, Avg };
    			DeclareEnumUtils(Type);
    
			VelocityDesc();
			VelocityDesc(Type);

    Type		type_;
    StaticsDesc		statics_;

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelocityType();
    static const char*	sKeyIsVelocity();
};


#endif
