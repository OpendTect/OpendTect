#ifndef uimadbldcmd_h
#define uimadbldcmd_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: uimadbldcmd.h,v 1.7 2007-12-11 15:18:26 cvsbert Exp $
-*/

#include "uigroup.h"
class uiLineEdit;
class uiSplitter;
class uiComboBox;
class uiListBox;
class uiTextEdit;
namespace ODMad { class ProgDef; }


class uiMadagascarBldCmd : public uiGroup
{
public:

			uiMadagascarBldCmd(uiParent*);
			~uiMadagascarBldCmd();

    void		setCmd(const char*);

    Notifier<uiMadagascarBldCmd> cmdAvailable;
    bool		isAdd() const	{ return cmdisadd_; }
    const char*		command() const;

protected:

    bool		cmdisadd_;

    uiComboBox*		groupfld_;
    uiListBox*		progfld_;
    uiLineEdit*		cmdfld_;
    uiLineEdit*		descfld_;
    uiLineEdit*		synopsfld_;
    uiLineEdit*		srchfld_;
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

    void		setProgName(const char*);
    void		setInput(const ODMad::ProgDef*);
    const ODMad::ProgDef* getDef(const char*);
    void		setGroupProgs(const BufferString*);

};


#endif
