#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.2 2007-09-17 12:37:37 cvskris Exp $
________________________________________________________________________

*/

#include "enums.h"

class IOPar;

//!Specifies velocity type and which z-interval a velocity sample belongs to

class VelocityDesc
{
public:
    enum Type		{ Unknown, Interval, RMO, Avg };
    			DeclareEnumUtils(Type);
    enum SampleRange	{ Centered, Above, Below };
    			DeclareEnumUtils(SampleRange);
    
			VelocityDesc();
			VelocityDesc(Type,SampleRange);

    Type		type_;
    SampleRange		samplerange_;

    BufferString	toString() const;
    bool		fromString(const char*);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    static const char*	sKeyVelocityDesc();
};


#endif
