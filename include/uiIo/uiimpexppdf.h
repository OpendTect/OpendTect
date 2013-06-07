#ifndef uiimpexppdf_h
#define uiimpexppdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra / Bert
 Date:          Jan 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class Sampled2DProbDenFunc;

/*! \brief Dialog for RokDoc PDF Import
    Imports Probability density functions in RokDoc ASCII format
*/

mClass uiImpRokDocPDF : public uiDialog
{
public:
			uiImpRokDocPDF(uiParent*);

protected:

    uiFileInput*	inpfld_;
    uiGenInput*		varnmsfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		xnrbinfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		ynrbinfld_;
    uiIOObjSel*		outputfld_;

    void		selChg(CallBacker*);
    void		extPDF(CallBacker*);

    Sampled2DProbDenFunc* getAdjustedPDF(Sampled2DProbDenFunc*);
    bool		acceptOK(CallBacker*);

};


/*! \brief Dialog for RokDoc PDF Export
    Exports Probability density functions in RokDoc ASCII format
*/

mClass uiExpRokDocPDF : public uiDialog
{
public:
			uiExpRokDocPDF(uiParent*);

protected:

    uiIOObjSel*		inpfld_;
    uiFileInput*	outfld_;

    bool		acceptOK(CallBacker*);

};


#endif
