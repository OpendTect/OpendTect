#ifndef uifontsel_H
#define uifontsel_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          25/9/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uidialog.h"
#include "bufstringset.h"

class uiButton;
class uiLabeledComboBox;
class uiFont;
class FontData;


mGlobal(uiBase) bool select( uiFont&, uiParent* parnt=0,
			     const uiString& title=0 );
	/*!< \brief pops a selector box to select a new font
	     \return true if new font selected
	*/

mGlobal(uiBase) bool select( FontData&,uiParent* parnt=0,
			     const uiString& title = 0);

mExpClass(uiBase) uiSetFonts : public uiDialog
{
public:

		uiSetFonts(uiParent*,const uiString& title);

protected:

    ObjectSet<uiButton>	buttons;

    void	butPushed(CallBacker*);

};


mExpClass(uiBase) uiSelFonts : public uiDialog
{
public:

		uiSelFonts(uiParent*,const uiString& title,const HelpKey&);
		~uiSelFonts();

    void	add(const char* str,const char* stdfontkey);

    const char*	resultFor(const char* str);

protected:

    ObjectSet<uiLabeledComboBox>	sels;
    BufferStringSet			ids;

};


#endif

