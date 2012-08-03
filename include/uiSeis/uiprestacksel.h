#ifndef uiprestacksel_h
#define uiprestacksel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
 RCS:           $Id: uiprestacksel.h,v 1.2 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "datapack.h"
#include "multiid.h"
#include "uidialog.h"
#include "uigroup.h"

class CtxtIOObj;
class uiIOSelect;
class uiListBox;
class uiSeisSel;

mClass(uiSeis) uiPreStackDataPackSelDlg : public uiDialog
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


mClass(uiSeis) uiPreStackSel : public uiGroup
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

