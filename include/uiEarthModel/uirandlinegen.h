#ifndef uirandlinegen_h
#define uirandlinegen_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2007
 RCS:           $Id: uirandlinegen.h,v 1.1 2007-11-15 16:54:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;


/*! \brief Generate random lines from contours of a horizon */

class uiGenRanLinesByContour : public uiDialog
{
public:
			uiGenRanLinesByContour(uiParent*);
			~uiGenRanLinesByContour();

    const char*		getNewSetID() const;

protected:

    CtxtIOObj&		horctio_;
    CtxtIOObj&		rlsctio_;

    uiIOObjSel*		infld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		contzrgfld_;
    uiGenInput*		relzrgfld_;
    uiGenInput*		abszrgfld_;
    uiCheckBox*		isrelfld_;

    void		isrelChg(CallBacker*);

    bool		acceptOK(CallBacker*);

};


#endif
