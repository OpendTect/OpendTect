#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2007
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class uiSelZRange;
class uiLabeledSpinBox;
class uiSpinBox;
class uiLabel;


/*! \brief Generate random lines from contours of a horizon */

mExpClass(uiEarthModel) uiGenRanLinesByContour : public uiDialog
{ mODTextTranslationClass(uiGenRanLinesByContour);
public:
			uiGenRanLinesByContour(uiParent*);
			~uiGenRanLinesByContour();

    MultiID		getNewSetID() const;
    bool		dispOnCreation();

protected:

    CtxtIOObj&		horctio_;
    CtxtIOObj&		polyctio_;
    CtxtIOObj&		rlsctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		polyfld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		contzrgfld_;
    uiGenInput*		relzrgfld_;
    uiGenInput*		abszrgfld_;
    uiCheckBox*		isrelfld_;
    uiCheckBox*		dispfld_;
    uiLabeledSpinBox*	vtxthreshfld_;
    uiCheckBox*		largestfld_;
    uiSpinBox*		nrlargestfld_;
    uiLabel*		largestendfld_;

    void		largestOnlyChg(CallBacker*);
    void		isrelChg(CallBacker*);

    bool		acceptOK(CallBacker*);

public:
    static uiString	sSpecGenPar();
    static uiString	sDlgTitle();
};


/*! \brief Generate random lines by shifting an existing */

mExpClass(uiEarthModel) uiGenRanLinesByShift : public uiDialog
{ mODTextTranslationClass(uiGenRanLinesByShift);
public:
			uiGenRanLinesByShift(uiParent*);
			~uiGenRanLinesByShift();

    MultiID		getNewSetID() const;
    bool		dispOnCreation();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		distfld_;
    uiGenInput*		sidefld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK(CallBacker*);

};


/*! \brief Generate random line from polygon */

mExpClass(uiEarthModel) uiGenRanLineFromPolygon : public uiDialog
{ mODTextTranslationClass(uiGenRanLineFromPolygon);
public:
			uiGenRanLineFromPolygon(uiParent*);
			~uiGenRanLineFromPolygon();

    MultiID		getNewSetID() const;
    bool		dispOnCreation();

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiSelZRange*	zrgfld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK(CallBacker*);

};


