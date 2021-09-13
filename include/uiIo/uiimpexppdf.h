#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Jan 2010
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiToolButton;
class uiUnitSel;
class ArrayNDProbDenFunc;
class Sampled1DProbDenFunc;
class Sampled2DProbDenFunc;

/*! \brief Dialog for RokDoc PDF Import
    Imports Probability density functions in RokDoc ASCII format
*/

mExpClass(uiIo) uiImpRokDocPDF : public uiDialog
{ mODTextTranslationClass(uiImpRokDocPDF);
public:
			uiImpRokDocPDF(uiParent*);

protected:

    uiFileInput*	inpfld_;
    uiGenInput*		varnmsfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		xnrbinfld_;
    uiUnitSel*		xunitfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		ynrbinfld_;
    uiUnitSel*		yunitfld_;
    uiToolButton*	extendbut_;
    uiIOObjSel*		outputfld_;

    void		selChg(CallBacker*);
    void		extPDF(CallBacker*);

    bool		acceptOK(CallBacker*);

    ArrayNDProbDenFunc* getAdjustedPDF(ArrayNDProbDenFunc*) const;
    void		setDisplayedFields(bool dim1,bool dim2);

};


/*! \brief Dialog for RokDoc PDF Export
    Exports Probability density functions in RokDoc ASCII format
*/

mExpClass(uiIo) uiExpRokDocPDF : public uiDialog
{ mODTextTranslationClass(uiExpRokDocPDF);
public:
			uiExpRokDocPDF(uiParent*);

protected:

    uiIOObjSel*		inpfld_;
    uiFileInput*	outfld_;

    bool		acceptOK(CallBacker*);

};


