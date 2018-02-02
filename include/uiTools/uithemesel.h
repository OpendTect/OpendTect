#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistring.h"

class uiComboBox;


/*!<\brief Theme selector for OpendTect. Shows result immediately. */


mExpClass(uiTools) uiThemeSel : public uiGroup
{ mODTextTranslationClass(uiThemeSel);
public:

			uiThemeSel(uiParent*,bool withlabel);

    bool		putInSettings(bool writesettings);
			//!< returns whether a new style was selected
    void		revert();
			//!< Makes sure style is restored

protected:

    BufferString	themenameatentry_;
    BufferStringSet	themenames_;

    uiComboBox*		selfld_;

    void		themeSel(CallBacker*);
    static void		activateTheme(const char*);

public:

    static bool		setODTheme(const char* themeid,bool writesetts);
			//!< Just so you can (but you probably don't want to)
			//!< returns whether themeid is a style change

};
