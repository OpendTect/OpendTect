#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "uigroup.h"

class uiCheckBox;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiToolButton;
class CommandDefs;

namespace sKey {
    inline StringView ExeName()	{ return "ExeName"; }
    inline StringView Command()	{ return "Command"; }
    inline StringView Arguments()	{ return "Arguments"; }
    inline StringView ToolTip()	{ return "ToolTip"; }
    inline StringView IconFile()	{ return "IconFile"; }
};


mExpClass(uiTools) uiToolBarCommandEditor : public uiGroup
{  mODTextTranslationClass(uiToolBarCommandEditor);
public:
    uiToolBarCommandEditor(uiParent*, const uiString&, const CommandDefs&,
			   bool withcheck=true, bool mkinvisible=false);
    uiToolBarCommandEditor(uiParent*, const uiString&,
			   const BufferStringSet& paths,
			   const BufferStringSet& exenms,
			   bool withcheck=true, bool mkinvisible=false);
    ~uiToolBarCommandEditor();

    void		setChecked(bool);
    bool		isChecked() const;

    void		updateCmdList(const CommandDefs&);
    void		clear();
    BufferString	getCommand() const;
    BufferString	getToolTip() const;
    BufferString	getIconFile() const;
    void		setCommand(const BufferString&);
    void		setToolTip(const BufferString&);
    void		setIconFile(const BufferString&);

    void		advSetSensitive(bool);
    void		advDisplay(bool);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    Notifier<uiToolBarCommandEditor> checked;
    Notifier<uiToolBarCommandEditor> changed;

protected:
    uiCheckBox*		checkbox_=nullptr;
    bool		mkinvisible_;
    uiComboBox*		exeselfld_ = nullptr;
    uiFileInput*	commandfld_;
    uiGenInput*		tooltipfld_;
    uiToolButton*	iconfld_;
    BufferString	iconfile_;
    CommandDefs&	commands_;

    void		initui(const uiString&, const BufferStringSet&, bool);

    void		checkCB(CallBacker*);
    void		commandChgCB(CallBacker*);
    void		exeSelChgCB(CallBacker*);
    void		initGrp(CallBacker*);
    void		iconSelCB(CallBacker*);

};
