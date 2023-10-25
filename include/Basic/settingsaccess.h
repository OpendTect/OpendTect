#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "settings.h"


/*!\brief Functionality to access specific user settings and/or environment
	  variables from different parts of OpendTect in order to avoid
	  duplication of definitions and code. */


mExpClass(Basic) SettingsAccess
{
public:
			SettingsAccess();	// accesses Settings::common()
			SettingsAccess(Settings&);

    bool		doesUserWantShading(bool forvolumes=false) const;

    int			getDefaultTexResFactor(int nrres) const;

    static bool		systemHasDefaultTexResFactor();

    int			getDefaultTexResAsIndex(int nrres) const;
    void		setDefaultTexResAsIndex(int idx,int nrres);
			//!<system default represented by idx==nrres

    BufferString	getHostNameOverrule() const;
    void		setHostNameOverrule(const char*);

    static const char*	sKeyIcons();
    static const char*	sKeyShowInlProgress();
    static const char*	sKeyShowCrlProgress();
    static const char*	sKeyShowZProgress();
    static const char*	sKeyShowRdlProgress();
    static const char*	sKeyTexResFactor();
    static const char*	sKeyUseSurfShaders();
    static const char*	sKeyUseVolShaders();
    static const char*	sKeyEnableMipmapping();
    static const char*	sKeyAnisotropicPower();
    static const char*	sKeyMouseWheelReversal();
    static const char*	sKeyMouseWheelZoomFactor();
    static const char*	sKeyHostNameOverrule();
    static const char*	sKeyDefaultAmbientReflectivity();
    static const char*	sKeyDefaultDiffuseReflectivity();

protected:
    Settings&			settings_;
};
