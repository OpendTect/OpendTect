#ifndef uirandlinegen_h
#define uirandlinegen_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2007
 RCS:           $Id: uirandlinegen.h,v 1.3 2008-02-14 09:45:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;
class uiSelZRange;


/*! \brief Generate random lines from contours of a horizon */

class uiGenRanLinesByContour : public uiDialog
{
public:
			uiGenRanLinesByContour(uiParent*);
			~uiGenRanLinesByContour();

    const char*		getNewSetID() const;

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

    void		isrelChg(CallBacker*);

    bool		acceptOK(CallBacker*);

};


/*! \brief Generate random lines by shifting an existing */

class uiGenRanLinesByShift : public uiDialog
{
public:
			uiGenRanLinesByShift(uiParent*);
			~uiGenRanLinesByShift();

    const char*		getNewSetID() const;

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		distfld_;
    uiGenInput*		sidefld_;

    bool		acceptOK(CallBacker*);

};


/*! \brief Generate random line from polygon */

class uiGenRanLineFromPolygon : public uiDialog
{
public:
			uiGenRanLineFromPolygon(uiParent*);
			~uiGenRanLineFromPolygon();

    const char*		getNewSetID() const;

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiSelZRange*	zrgfld_;

    bool		acceptOK(CallBacker*);

};


#endif
