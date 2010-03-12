#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id: veldesc.h,v 1.13 2010-03-12 13:42:03 cvskris Exp $
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

    bool		operator==(const VelocityDesc&) const;
    bool		operator!=(const VelocityDesc&) const;

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelocityType();
    static const char*	sKeyIsFeetPerSecond();
    static const char*	sKeyIsVelocity();

    static const char*	getVelUnit(bool withparens=true);
};


#endif
