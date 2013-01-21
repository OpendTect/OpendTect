#ifndef veldesc_h
#define veldesc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "enums.h"
#include "multiid.h"
#include "staticsdesc.h"


/*!
\ingroup Basic
\brief Specifies velocity type and statics for a velocity.
*/

mExpClass(Basic) VelocityDesc
{
public:
    enum Type		{ Unknown, Interval, RMS, Avg, Delta, Epsilon, Eta };
    			DeclareEnumUtils(Type);
    
			VelocityDesc();
			VelocityDesc(Type);

    Type		type_;
    StaticsDesc		statics_;

    bool		operator==(const VelocityDesc&) const;
    bool		operator!=(const VelocityDesc&) const;

    static bool		isVelocity(Type);
			//!<\returns true if not unknown or a Thomsen parameter
    bool		isVelocity() const;
			//!<\returns true if not unknown or a Thomsen parameter
    static bool		isThomsen(Type);
			//!<\returns true if not unknown or a Velocity
    bool		isThomsen() const;
			//!<\returns true if not unknown or a Velocity

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelocityType();
    static const char*	sKeyIsFeetPerSecond();
    static const char*	sKeyIsVelocity();

    static const char*	getVelUnit(bool withparens=true);
};


#endif

