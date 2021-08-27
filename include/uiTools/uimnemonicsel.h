#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          Aug 2021
________________________________________________________________________

-*/


#include "uitoolsmod.h"

#include "uicombobox.h"
#include "uistrings.h"

#include "mnemonics.h"


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


