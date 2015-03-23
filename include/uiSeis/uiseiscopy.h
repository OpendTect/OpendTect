#ifndef uiseiscopy_h
#define uiseiscopy_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class IOObj;
class uiSeisSel;
class uiScaler;
class uiSeis2DMultiLineSel;
class uiLabeledComboBox;
class uiSeisTransfer;
class uiBatchJobDispatcherSel;


/*!\brief UI for copying cubes */

mExpClass(uiSeis) uiSeisCopyCube : public uiDialog
{ mODTextTranslationClass(uiSeisCopyCube);
public:

			uiSeisCopyCube(uiParent*,const IOObj*);

protected:

    uiSeisSel*		inpfld_;
    uiLabeledComboBox*	compfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel* batchfld_;

    bool		ismc_;

    void		inpSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};


/*!\brief UI for copying line sets */

mExpClass(uiSeis) uiSeisCopyLineSet : public uiDialog
{
public:

			uiSeisCopyLineSet(uiParent*,const IOObj*);
protected:

    uiSeisSel*		inpfld_;
    uiSeis2DMultiLineSel* subselfld_;
    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;
    uiBatchJobDispatcherSel* batchfld_;

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

