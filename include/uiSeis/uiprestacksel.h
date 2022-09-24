#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "datapack.h"
#include "multiid.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

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
