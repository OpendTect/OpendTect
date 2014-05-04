#ifndef uifontsel_h
#define uifontsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          25/9/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uisettings.h"
#include "bufstringset.h"

class uiButton;
class uiLabeledComboBox;
class uiFont;
class FontData;

mExpClass(uiTools) uiFontSettingsGroup : public uiSettingsGroup
{
public:
			mDefaultFactoryInstantiation2Param(
				uiSettingsGroup,
				uiFontSettingsGroup,
				uiParent*,Settings&,
				"Fonts",
				sFactoryKeyword())

			uiFontSettingsGroup(uiParent*,Settings&);

protected:

    ObjectSet<uiButton>	buttons;

    void		butPushed(CallBacker*);
};


mExpClass(uiTools) uiSelFonts : public uiDialog
{
public:

			uiSelFonts(uiParent*,const uiString& title,
				   const HelpKey&);
			~uiSelFonts();

    void		add(const char* str,const char* stdfontkey);

    const char*		resultFor(const char* str);

protected:

    ObjectSet<uiLabeledComboBox>	sels;
    BufferStringSet			ids;

};

#endif

