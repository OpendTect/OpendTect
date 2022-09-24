#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

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

    bool		acceptOK(CallBacker*) override;

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

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		distfld_;
    uiGenInput*		sidefld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK(CallBacker*) override;

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

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiSelZRange*	zrgfld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK(CallBacker*) override;

};
