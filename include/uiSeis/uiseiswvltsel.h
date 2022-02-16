#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "bufstringset.h"

class Wavelet;
class uiComboBox;
class uiWaveletExtraction;

/*!\brief 'Immediate' Wavelet selector, with optionally 'Manage' and/or
  'Extract' buttons */

mExpClass(uiSeis) uiSeisWaveletSel : public uiGroup
{ mODTextTranslationClass(uiSeisWaveletSel)
public:

    mExpClass(uiSeis) Setup
    {
	public:
			Setup(const char* seltxt="Wavelet")
			    : seltxt_(seltxt)
			    , withextract_(true)
			    , withman_(true)
			    , compact_(false)
			{}

	mDefSetupMemb(BufferString,seltxt);
	mDefSetupMemb(bool,withextract);
	mDefSetupMemb(bool,withman);
	mDefSetupMemb(bool,compact);
    };

			uiSeisWaveletSel(uiParent*,const Setup& =Setup());
			uiSeisWaveletSel(uiParent*,
					 const char* seltxt,
					 bool withextract=true,
					 bool withman=true,
					 bool compact=false);
			~uiSeisWaveletSel();

    void		rebuildList();

    const MultiID&	getID() const;
    Wavelet*		getWavelet() const;
    const char*		getWaveletName() const;
    void		setInput(const char*);
    void		setInput(const MultiID&);

    Notifier<uiSeisWaveletSel> newSelection;

protected:

    uiComboBox*		nmfld_;
    BufferStringSet	nms_;
    ObjectSet<MultiID>	ids_;

    uiWaveletExtraction* wvltextrdlg_ = nullptr;

    void		initFlds(CallBacker*);
    void		extractCB(CallBacker*);
    void		extractionDoneCB(CallBacker*);
    void		startMan(CallBacker*);
    void		selChg(CallBacker*);

};


mExpClass(uiSeis) uiWaveletSel : public uiIOObjSel
{ mODTextTranslationClass(uiWaveletSel)
public:
			uiWaveletSel(uiParent*,bool forread,
				     const uiIOObjSel::Setup&);
			uiWaveletSel(uiParent*,bool forread);
			~uiWaveletSel();

    Wavelet*		getWavelet(bool noerr) const;
};
