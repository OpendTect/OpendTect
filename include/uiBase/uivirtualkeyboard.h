#ifndef uivirtualkeyboard_h
#define uivirtualkeyboard_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          October 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uimainwin.h"


class uiGraphicsViewBase;
class uiLineEdit;
class uiGraphicsItemSet;


mClass(uiBase) uiVirtualKeyboard : public uiMainWin
{
public:
    				uiVirtualKeyboard(uiObject&,int x,int y);
				~uiVirtualKeyboard();

    bool			enterPressed() const;

    static bool			isVirtualKeyboardActive();

protected:

    void			clickCB(CallBacker*);
    void			exitCB(CallBacker*);

    void			updateKeyboard();
    void			updateInputObj();

    uiObject&			inputobj_;
    int				globalx_;
    int				globaly_;
    float			keyboardscale_;

    uiGraphicsViewBase*		viewbase_;
    uiLineEdit*			textline_;

    void			addLed(float x,float y,const Color&);
    void			updateLeds();
    uiGraphicsItemSet*		leds_;

    bool			capslock_;
    bool			shiftlock_;
    bool			ctrllock_;
    bool			altlock_;
    bool			shift_;
    bool			ctrl_;
    bool			alt_;

    void			enterCB(CallBacker*);
    bool			enterpressed_;

    void			selChg(CallBacker*);
    void			restoreSelection();
    int				selectionstart_;
    int				selectionlength_;
};


#endif

