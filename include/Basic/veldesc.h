#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.6 2008-12-18 05:23:26 cvsranojay Exp $
________________________________________________________________________

*/

#include "enums.h"

class IOPar;

//!Specifies velocity type and which z-interval a velocity sample belongs to

mClass VelocityDesc
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
