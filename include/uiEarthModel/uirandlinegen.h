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
#include "dbkey.h"

class uiGenInput;
class uiIOObjSel;
class uiPickSetIOObjSel;
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

    DBKey		getNewSetID() const;
    bool		dispOnCreation();

protected:

    uiIOObjSel*		infld_;
    uiPickSetIOObjSel*	polyfld_;
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

    bool		acceptOK();

public:
    static uiString	sSpecGenPar();
    static uiString	sDlgTitle();
};


/*! \brief Generate random lines by shifting an existing */

mExpClass(uiEarthModel) uiGenRanLinesByShift : public uiDialog
{ mODTextTranslationClass(uiGenRanLinesByShift);
public:
			uiGenRanLinesByShift(uiParent*);

    DBKey		getNewSetID() const;
    bool		dispOnCreation();

protected:

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		distfld_;
    uiGenInput*		sidefld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK();

};


/*! \brief Generate random line from polygon */

mExpClass(uiEarthModel) uiGenRanLineFromPolygon : public uiDialog
{ mODTextTranslationClass(uiGenRanLineFromPolygon);
public:
			uiGenRanLineFromPolygon(uiParent*);

    DBKey		getNewSetID() const;
    bool		dispOnCreation();

protected:

    uiPickSetIOObjSel*	infld_;
    uiIOObjSel*		outfld_;
    uiSelZRange*	zrgfld_;
    uiCheckBox*		dispfld_;

    bool		acceptOK();

};
