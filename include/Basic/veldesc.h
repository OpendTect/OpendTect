#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.5 2008-12-08 20:10:14 cvskris Exp $
________________________________________________________________________

*/

#include "enums.h"

class IOPar;

//!Specifies velocity type and which z-interval a velocity sample belongs to

class VelocityDesc
{
public:
    enum Type		{ Unknown, Interval, RMS, Avg };
    			DeclareEnumUtils(Type);
    enum SampleSpan	{ Centered, Above, Below };
    			DeclareEnumUtils(SampleSpan);
    
			VelocityDesc();
			VelocityDesc(Type,SampleSpan);

    Type		type_;
    SampleSpan		samplespan_;

    BufferString	toString() const;
    bool		fromString(const char*);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    static const char*	sKeyVelocityDesc();
    static const char*	sKeyIsVelocity();
};


#endif
