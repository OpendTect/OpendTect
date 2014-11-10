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

#include "uiiomod.h"
#include "uidialog.h"
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class ArrayNDProbDenFunc;
class Sampled1DProbDenFunc;
class Sampled2DProbDenFunc;

/*! \brief Dialog for RokDoc PDF Import
    Imports Probability density functions in RokDoc ASCII format
*/

mExpClass(uiIo) uiImpRokDocPDF : public uiDialog
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

			/* Do not use, obsoleted after 5.0 */
    Sampled2DProbDenFunc* getAdjustedPDF(Sampled2DProbDenFunc*);
    bool		acceptOK(CallBacker*);

    ArrayNDProbDenFunc*	getAdjustedPDF(ArrayNDProbDenFunc*) const;
    void		setDisplayedFields(bool dim1,bool dim2);

};


/*! \brief Dialog for RokDoc PDF Export
    Exports Probability density functions in RokDoc ASCII format
*/

mExpClass(uiIo) uiExpRokDocPDF : public uiDialog
{
public:
			uiExpRokDocPDF(uiParent*);

protected:

    uiIOObjSel*		inpfld_;
    uiFileInput*	outfld_;

    bool		acceptOK(CallBacker*);

};


#endif

