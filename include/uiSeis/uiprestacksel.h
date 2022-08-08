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
#include "multiid.h"
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
				const MultiID& selid);

    const MultiID&	getMultiID() const	{ return selid_; }

protected:

    uiListBox*		datapackinpfld_;
    const TypeSet<DataPack::FullID>& dpfids_;
    MultiID		selid_;

    bool		acceptOK(CallBacker*) override;
};


mExpClass(uiSeis) uiPreStackSel : public uiGroup
{ mODTextTranslationClass(uiPreStackSel);
public:

			uiPreStackSel(uiParent*,bool is2d);

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		setInput(const MultiID&);
    MultiID		getMultiID() const;

    void		setDataPackInp(const TypeSet<DataPack::FullID>& ids);
    bool		commitInput();

protected:

    uiSeisSel*		seisinpfld_;
    uiIOSelect*		datapackinpfld_;

    void		doSelDataPack(CallBacker*);

    MultiID		selid_;
    TypeSet<DataPack::FullID> dpfids_;
};


