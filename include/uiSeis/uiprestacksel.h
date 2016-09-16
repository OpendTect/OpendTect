#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "datapack.h"
#include "dbkey.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

class CtxtIOObj;
class uiIOSelect;
class uiListBox;
class uiSeisSel;

mExpClass(uiSeis) uiPreStackDataPackSelDlg : public uiDialog
{ mODTextTranslationClass(uiPreStackDataPackSelDlg);
public:

			uiPreStackDataPackSelDlg(uiParent*,
				const TypeSet<DataPack::FullID>& dpfids,
				const DBKey& selid);

    const DBKey&	getDBKey() const	{ return selid_; }

protected:

    uiListBox*		datapackinpfld_;
    const TypeSet<DataPack::FullID>& dpfids_;
    DBKey		selid_;

    bool		acceptOK();
};


mExpClass(uiSeis) uiPreStackSel : public uiGroup
{ mODTextTranslationClass(uiPreStackSel);
public:

			uiPreStackSel(uiParent*,bool is2d);

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		setInput(const DBKey&);
    DBKey		getDBKey() const;

    void		setDataPackInp(const TypeSet<DataPack::FullID>& ids);
    bool		inputOK();

protected:

    uiSeisSel*		seisinpfld_;
    uiIOSelect*		datapackinpfld_;

    void		doSelDataPack(CallBacker*);

    TypeSet<DataPack::FullID> dpfids_;

};
