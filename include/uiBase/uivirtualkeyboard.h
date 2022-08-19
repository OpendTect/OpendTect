#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uimainwin.h"


class uiGraphicsViewBase;
class uiLineEdit;
class uiGraphicsItemSet;


mExpClass(uiBase) uiVirtualKeyboard : public uiMainWin
{ mODTextTranslationClass(uiVirtualKeyboard);
public:
    				uiVirtualKeyboard(uiObject&,int x,int y);
				~uiVirtualKeyboard();

    bool			enterPressed() const;

    static bool			isVirtualKeyboardActive();

    static const char*		sKeyEnabVirtualKeyboard();
    static bool			isVirtualKeyboardEnabled();

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

    void			addLed(float x,float y,const OD::Color&);
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
