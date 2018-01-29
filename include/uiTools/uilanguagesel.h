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


mExpClass(uiTools) uiLanguageSel : public uiGroup
{ mODTextTranslationClass(uiLanguageSel);
public:

			uiLanguageSel(uiParent*,bool withlbl);
			~uiLanguageSel();

    uiComboBox*		selFld()	{ return selfld_; }
    void		setAutoCommit( bool yn=true )
					{ autocommit_ = yn; }

    bool		commit(bool writesettings);

    static bool		haveMultipleLanguages();

protected:

    uiComboBox*		selfld_;
    bool		autocommit_;

    void		langSelCB(CallBacker*);
    void		extLangChgCB(CallBacker*);
    static void		activateTheme(const char*);

public:

    static bool		setODLocale(const char* localename,bool writesetts);
			//!< Just so you can (but you probably don't want to)

};
