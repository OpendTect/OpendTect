#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

#include "uigroup.h"

class uiCheckBox;
class uiComboBox;
class uiLineEdit;
class uiListBox;
class uiMadagascarBldPlotCmd;
class uiTextEdit;
namespace ODMad { class ProgDef; class Proc; }


mClass(uiMadagascar) uiMadagascarBldCmd : public uiGroup
{ mODTextTranslationClass(uiMadagascarBldCmd);
public:

			uiMadagascarBldCmd(uiParent*);
			~uiMadagascarBldCmd();

    void		setProc(const ODMad::Proc*);

    Notifier<uiMadagascarBldCmd> cmdAvailable;
    bool		isAdd() const	{ return cmdisadd_; }
    ODMad::Proc*	proc() const;

protected:

    bool		cmdisadd_;

    uiComboBox*		groupfld_;
    uiListBox*		progfld_;
    uiLineEdit*		cmdfld_;
    uiMadagascarBldPlotCmd*		auxcmdfld_;
    uiLineEdit*		descfld_;
    uiLineEdit*		synopsfld_;
    uiLineEdit*		srchfld_;
    uiCheckBox*		useauxfld_;
    uiTextEdit*		commentfld_;

    void		createMainPart(uiGroup*);
    uiGroup*		createLowGroup();

    void		onPopup(CallBacker*);
    void		groupChg(CallBacker*);
    void		progChg(CallBacker*);
    void		doEdit(CallBacker*);
    void		doAdd(CallBacker*);
    void		dClick(CallBacker*);
    void		doSearch(CallBacker*);
    void		auxSel(CallBacker*);

    void		setProgName(const char*);
    void		setInput(const ODMad::ProgDef*);
    const ODMad::ProgDef* getDef(const char*);
    void		setGroupProgs(const BufferString*);

};

