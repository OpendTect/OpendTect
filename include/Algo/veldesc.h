#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "odcommonenums.h"
#include "multiid.h"
#include "staticsdesc.h"
#include "uistring.h"

namespace ZDomain { class Def; }


/*!
Specifies velocity type and statics for a velocity.

To tag a velocity volume as a velocity, this class can be used to do the work:

\code
    const VelocityDesc desc( OD::VelocityType::Interval );

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
			VelocityDesc(OD::VelocityType,
				     const char* unitstr =nullptr);

    Type		type_;
    OD::VelocityType	getType() const;
    void		setType(OD::VelocityType);
    StaticsDesc		statics_;
    BufferString	velunit_;
    bool		operator==(const VelocityDesc&) const;
    bool		operator!=(const VelocityDesc&) const;

    static bool		isUdf(Type);
    static bool		isUdf(OD::VelocityType);
    bool		isUdf() const;
    static bool		isVelocity(Type);
    static bool		isVelocity(OD::VelocityType);
			//!<\returns true if not unknown or a Thomsen parameter
    bool		isVelocity() const;
			//!<\returns true if not unknown or a Thomsen parameter
    bool		isInterval() const;
    bool		isRMS() const;
    bool		isAvg() const;
    static bool		isThomsen(Type);
    static bool		isThomsen(OD::VelocityType);
			//!<\returns true if not unknown or a Velocity
    bool		isThomsen() const;
			//!<\returns true if not unknown or a Velocity

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyVelocityType();
    mDeprecatedDef
    static const char*	sKeyIsFeetPerSecond();
    mDeprecatedDef
    static const char*	sKeyIsVelocity();
    static const char*	sKeyVelocityVolume();
    static const char*	sKeyVelocityUnit();

    mDeprecatedDef
    static uiString	getVelUnit(bool withparens=true);
    static uiString	getVelVolumeLabel();

    static bool		isUsable(Type,const ZDomain::Def&,uiRetVal&);
    static bool		isUsable(OD::VelocityType,const ZDomain::Def&,
				 uiRetVal&);

    static Type		get(OD::VelocityType);
    static OD::VelocityType get(Type);
};
