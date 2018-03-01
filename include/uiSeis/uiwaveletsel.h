#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2016
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiioobjsel.h"
#include "wavelet.h"
#include "ioobjctxt.h"
class uiWaveletExtraction;


/*!\brief selects wavelets. */

mExpClass(uiSeis) uiWaveletIOObjSel : public uiIOObjSel
{ mODTextTranslationClass(uiWaveletIOObjSel);
public:

    mExpClass(uiSeis) Setup : public uiIOObjSel::Setup
    {
    public:
			Setup( const uiString& seltext=uiString::empty() )
			    : uiIOObjSel::Setup(seltext)
			    , withextract_(true)
			    , withman_(true)		{}

			    // Only when for read:
	mDefSetupMemb(bool,withextract)
	mDefSetupMemb(bool,withman)

    };

			uiWaveletIOObjSel(uiParent*,bool forread=true);
			uiWaveletIOObjSel(uiParent*,const Setup&,
					  bool forread=true);

    ConstRefMan<Wavelet> getWavelet() const;
    RefMan<Wavelet>	getWaveletForEdit() const;
    bool		store(const Wavelet&,bool askoverwrite=true);

    static IOObjContext	getCtxt(bool forread=true);

protected:

    uiWaveletExtraction* extrdlg_;

    void		startManCB(CallBacker*);
    void		extractCB(CallBacker*);
    void		extrDlgCloseCB(CallBacker*);
    void		extractionDoneCB(CallBacker*);

private:

    void		init(const Setup&,bool);

};
