#ifndef uiseiswvltsel_h
#define uiseiswvltsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id$
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

			uiSeisWaveletSel(uiParent*,
					 const char* seltxt="Wavelet",
					 bool withextract=true,
					 bool withman=true,
					 bool compact=false);
			~uiSeisWaveletSel();
    void		rebuildList();

    const char*		getName() const;
    const MultiID&	getID() const;
    Wavelet*		getWavelet() const;
    void		setInput(const char*);
    void		setInput(const MultiID&);

    Notifier<uiSeisWaveletSel> newSelection;

protected:

    uiComboBox*		nmfld_;
    BufferStringSet	nms_;
    ObjectSet<MultiID>	ids_;

    uiWaveletExtraction* wvltextrdlg_;

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
    Wavelet*		getWavelet(bool noerr) const;
};

#endif
