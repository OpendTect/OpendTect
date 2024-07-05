#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uicombobox.h"
#include "uidialog.h"

#include "mnemonics.h"

class uiButtonGroup;
class uiListBox;
class uiMnSelFlds;
class uiTable;


/*!\brief Selector for Mnemonics */

mExpClass(uiTools) uiMnemonicsSel : public uiLabeledComboBox
{ mODTextTranslationClass(uiMnemonicsSel);
public:

    mExpClass(uiTools) Setup
    {
    public:

				Setup( Mnemonic::StdType typ,
				       const uiString labeltxt=defLabel() )
				    : lbltxt_(labeltxt)
				    , mnsel_(typ)
				{}
				Setup( const MnemonicSelection* mns = nullptr,
				       const uiString labeltxt=defLabel() )
				    : lbltxt_(labeltxt)
				    , mnsel_(nullptr)
				{
				    if ( mns )
					mnsel_ = *mns;
				}
	virtual			~Setup()
				{}

	static uiString		defLabel()	{ return tr("Mnemonic"); };

	mDefSetupMemb(MnemonicSelection,mnsel)
	mDefSetupMemb(uiString,lbltxt)
    };

				uiMnemonicsSel(uiParent*,const Setup&);
				uiMnemonicsSel(uiParent*,Mnemonic::StdType);
				uiMnemonicsSel(uiParent*,
					   const MnemonicSelection* =nullptr);
				~uiMnemonicsSel();

    uiMnemonicsSel*		clone() const;

    const Mnemonic*		mnemonic() const;
    Mnemonic::StdType		propType() const;
    const MnemonicSelection&	getSelection() const	{ return mns_; }

    void			setMnemonic(const Mnemonic&);
				//!< Sets current selection
    void			setNames(const BufferStringSet&);
				/*!< Replace the mnemonic names with others,
				 must match the box() size */
private:

    Setup			setup_;

    MnemonicSelection		mns_;
    BufferStringSet		altnms_;

    void			setFromSelection();

    void			init();

};


mExpClass(uiTools) uiMultiMnemonicsSel : public uiDialog
{ mODTextTranslationClass(uiMultiMnemonicsSel);
public:
				uiMultiMnemonicsSel(uiParent*,
					MnemonicSelection&,
					const MnemonicSelection* mnsel=nullptr);
				~uiMultiMnemonicsSel();

protected:

    uiListBox*			mnemlist_;
    MnemonicSelection&		mns_;

    bool			acceptOK(CallBacker*) override;

};


mExpClass(uiTools) uiCustomMnemonicsSel : public uiDialog
{ mODTextTranslationClass(uiCustomMnemonicsSel)
public:
				uiCustomMnemonicsSel(uiParent*);
				~uiCustomMnemonicsSel();

private:

    void			createTable();

    void			initDlg(CallBacker*);
    void			addRowButCB(CallBacker*);
    void			addRowTblRowCB(CallBacker*);
    void			removeButCB(CallBacker*);
    void			removeTblRowCB(CallBacker*);
    void			removeTblSelCB(CallBacker*);
    void			cellEditCB(CallBacker*);
    void			saveButCB(CallBacker*);

    void			resetTable();
    void			removeEntries(const TypeSet<int>& rows,
					      bool forreset=false);
    void			getEntries(ObjectSet<Mnemonic>&) const;
    bool			commitEntries(ObjectSet<Mnemonic>&);
    void			commitNameChanges();
    void			doRead();
    bool			doSave();

    bool			acceptOK(CallBacker*) override;

    ObjectSet<uiMnSelFlds>	selflds_;
    uiTable*			tbl_;
    uiButtonGroup*		bgrp_;

    bool			dontshowmgr_			= false;
    MnemonicSelection		originalcustommns_;
    MnemonicSelection		origentriesremoved_;
    ManagedObjectSet<std::pair<int,const BufferString>> newnames_;
};
