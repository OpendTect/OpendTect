#ifndef settingsaccess_h
#define settingsaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		01/12/2014
 RCS:		$Id$
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

    BufferString	getTerminalEmulator();

    static const char*	sKeyIcons();
    static const char*	sKeyShowInlProgress();
    static const char*	sKeyShowCrlProgress();
    static const char*	sKeyShowRdlProgress();
    static const char*	sKeyTexResFactor();
    static const char*	sKeyUseSurfShaders();
    static const char*	sKeyUseVolShaders();
    static const char*	sKeyEnableMipmapping();
    static const char*	sKeyAnisotropicPower();
    static const char*	sKeyMouseWheelReversal();
    static const char*	sKeyMouseWheelZoomFactor();

protected:
    Settings&			settings_;
};

#endif
