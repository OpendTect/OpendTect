#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2016
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uidialog.h"
class IOObj;
class uiTextEdit;
class uiCheckList;
class uiGenInput;


mExpClass(uiODMain) uiAutoSave2RealObjDlg : public uiDialog
{ mODTextTranslationClass(uiAutoSave2RealObjDlg)
public:

			uiAutoSave2RealObjDlg(uiParent*,IOObj&,int curidx,
						int totalnr);
    bool		isCancel() const;

    static int		run4All(const char* hnm=0,const char* usrnm=0);
    			// Runs number of objects handled

protected:

    void		choiceSel(CallBacker*);
    bool		acceptOK();

    IOObj&		ioobj_;

    uiTextEdit*		infofld_;
    uiCheckList*	choicefld_;
    uiGenInput*		newnmfld_;

    static void		doRestore(IOObj&,const char*);

};
