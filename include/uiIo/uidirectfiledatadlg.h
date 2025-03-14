#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/



#include "uiiomod.h"

#include "uidialog.h"

class IOObj;
class uiFileInput;

/*!\brief UI for manipulating fille names/paths for a SEGYDirect data-store */

mExpClass(uiIo) uiEditDirectFileDataDlg : public uiDialog
{ mODTextTranslationClass(uiIo)
public:
			uiEditDirectFileDataDlg(uiParent*,const IOObj&);
			~uiEditDirectFileDataDlg();

protected:

    virtual void	prepareFileNames();
    virtual void	doDirSel();
    virtual void	createInterface();

    const IOObj&	ioobj_;
    bool		isusable_;
    BufferStringSet	filenames_;

    uiFileInput*	selfld_				= nullptr;

private:

    void		dirSelCB(CallBacker*);

};
