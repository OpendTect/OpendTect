#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.11 2001-07-18 16:14:17 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
#include <multiid.h>
class IOObj;
class CtxtIOObj;
class IODirEntryList;
class uiListBox;
class uiGenInput;

class uiIOObjSelAuxInfo
{
public:

			uiIOObjSelAuxInfo( uiGroupCreater* g=0, const char* t=0)
			: grpcr(g), trglobexpr(t)		{}
			uiIOObjSelAuxInfo( const uiIOObjSelAuxInfo& ai )
			: grpcr(ai.grpcr), trglobexpr(ai.trglobexpr),
			  editcb(ai.editcb)			{}

    uiGroupCreater*	grpcr;
    const char*		trglobexpr;
    CallBack		editcb; //!< specify only if non-standard

};


/*! \brief Dialog for selection of IOObjs */

class uiIOObjSelDlg : public uiDialog
{
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
				      uiIOObjSelAuxInfo* ai=0);
			~uiIOObjSelDlg();

    const IOObj*	ioObj() const		{ return ioobj; }
    const NamedNotifierList& notifiers() const	{ return notifs; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;
    CNotifier<uiIOObjSelDlg,MultiID> selchg;
    NamedNotifierList	notifs;

    uiListBox*		listfld;
    uiGenInput*		nmfld;
    uiGroup*		grp;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
};


/*! \brief UI element for selection of IOObjs */

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				   bool wthclear=false,uiIOObjSelAuxInfo* ai=0);
			~uiIOObjSel();

    bool		commitInput(bool mknew);

    void		updateInput(); //!< updates from CtxtIOObj
    void		processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    bool		existingTyped() const;
					//!< returns false is typed input is
					//!< not an existing IOObj name
    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    CtxtIOObj&		ctio;
    bool		forread;
    uiIOObjSelAuxInfo	auxinfo;
    IOPar&		iopar;

    void		doObjSel(CallBacker*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

};


#endif
