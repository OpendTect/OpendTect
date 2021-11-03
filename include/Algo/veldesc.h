#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
________________________________________________________________________

*/

#include "algomod.h"
#include "enums.h"
#include "multiid.h"
#include "staticsdesc.h"
#include "uistring.h"


/*!
Specifies velocity type and statics for a velocity.

To tag a velocity volume as a velocity, this class can be used to do the work:

\code
    VelocityDesc desc( VelocityDesc::Interval );

    PtrMan<IOObj> ioobj = IOM().get( multiid );
    desc.fillPar( ioobj->pars() );
    IOM().commitChanges( ioobj );

\endcode

*/

mExpClass(Algo) VelocityDesc
{ mODTextTranslationClass(VelocityDesc);
public:
    enum Type		{ Unknown, Interval, RMS, Avg, Delta, Epsilon, Eta };
			mDeclareEnumUtils(Type);
    
			VelocityDesc();
			VelocityDesc(Type);

    Type		type_;
    StaticsDesc		statics_;
    BufferString	velunit_;
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
    static const char*	sKeyVelocityVolume();
    static const char*	sKeyVelocityUnit();

    static uiString	getVelUnit(bool withparens=true);
    static uiString	getVelVolumeLabel();
};


