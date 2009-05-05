#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.8 2009-05-05 16:48:33 cvskris Exp $
________________________________________________________________________

*/

#include "enums.h"
#include "multiid.h"

class IOPar;

/*!Specifies velocity type and which z-interval a velocity sample belongs to
   The SampleSpan is only relevent at Interval Veloicty. */

mClass VelocityDesc
{
public:
    enum Type		{ Unknown, Interval, RMS, Avg };
    			DeclareEnumUtils(Type);
    
			VelocityDesc();
			VelocityDesc(Type);

    Type		type_;

    MultiID		staticshorizon_;
    float		staticsvel_;
    BufferString	staticsvelattrib_;	//attrib on statichorizon_
    						//if empty, use vel

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelocityType();
    static const char*	sKeyStaticsHorizon();
    static const char*	sKeyStaticsVelocity();
    static const char*	sKeyStaticsVelocityAttrib();

    static const char*	sKeyIsVelocity();
};


#endif
