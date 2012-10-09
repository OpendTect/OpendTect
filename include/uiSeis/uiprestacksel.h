#ifndef uiprestacksel_h
#define uiprestacksel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "datapack.h"
#include "multiid.h"
#include "uidialog.h"
#include "uigroup.h"

class CtxtIOObj;
class uiIOSelect;
class uiListBox;
class uiSeisSel;

mClass uiPreStackDataPackSelDlg : public uiDialog
{
public:
    			uiPreStackDataPackSelDlg(uiParent*,
				const TypeSet<DataPack::FullID>& dpfids,
				const MultiID& selid);

    const MultiID&	getMultiID() const 	{ return selid_; }

protected:
    uiListBox*		datapackinpfld_;
    const TypeSet<DataPack::FullID>& dpfids_;
    MultiID		selid_;

    bool		acceptOK(CallBacker*);
};


mClass uiPreStackSel : public uiGroup
{
public:

			uiPreStackSel(uiParent*,bool is2d);
			~uiPreStackSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void 		setInput(const MultiID&);
    const MultiID	getMultiID() const;

    void	 	setDataPackInp(const TypeSet<DataPack::FullID>& ids);
    bool		commitInput();

protected:

    uiSeisSel*		seisinpfld_;
    uiIOSelect*		datapackinpfld_;

    BufferString	errmsg_;
    CtxtIOObj&		ctio_;
    MultiID		selid_;

    void		doSelDataPack(CallBacker*);

    TypeSet<DataPack::FullID> dpfids_;
};


#endif
