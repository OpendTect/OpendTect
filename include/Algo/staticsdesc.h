#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"

#include "multiid.h"


/*!
\brief Specifies Statics as a horizon and either a horizon attribute or a
constant velocity. Velocity is always in m/s.
*/

mExpClass(Algo) StaticsDesc
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
