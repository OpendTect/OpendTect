#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		May 2020
 ________________________________________________________________________

-*/
#include "uitoolsmod.h"

#include "bufstring.h"
#include "keystrs.h"
#include "uigroup.h"

class uiCheckBox;
class uiComboBox;
class uiFileSel;
class uiGenInput;
class uiToolButton;

namespace sKey {
    inline FixedString ExeName()	{ return "ExeName"; }
    inline FixedString Command()	{ return "Command"; }
    inline FixedString Arguments()	{ return "Arguments"; }
    inline FixedString ToolTip()	{ return "ToolTip"; }
    inline FixedString IconFile()	{ return "IconFile"; }
};

mExpClass(uiTools) uiToolBarCommandEditor : public uiGroup
{  mODTextTranslationClass(uiToolBarCommandEditor);
public:
    uiToolBarCommandEditor(uiParent*, const uiString&,
			   const BufferStringSet& paths,
			   const BufferStringSet& exenms,
			   bool withcheck=true, bool mkinvisible=false);
    ~uiToolBarCommandEditor();

    void		setChecked(bool);
    bool		isChecked() const;
    BufferStringSet	createUiList( const BufferStringSet& paths,
				      const BufferStringSet& exenms );
    uiStringSet		createUiStrSet( const BufferStringSet& paths,
				      const BufferStringSet& exenms );

    void		updateCmdList( const BufferStringSet& paths,
				       const BufferStringSet& exenms );
    void		clear();
    BufferString	getCommand() const;
    BufferString	getArguments() const;
    BufferString	getToolTip() const;
    BufferString	getIconFile() const;
    void		setCommand(const BufferString&);
    void		setArguments(const BufferString&);
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
    uiFileSel*		commandfld_;
    uiGenInput*		argumentsfld_;
    uiGenInput*		tooltipfld_;
    uiToolButton*	iconfld_;
    BufferString	iconfile_;

    void		checkCB(CallBacker*);
    void		commandChgCB(CallBacker*);
    void		exeSelChgCB(CallBacker*);
    void		initGrp(CallBacker*);
    void		iconSelCB(CallBacker*);

};
