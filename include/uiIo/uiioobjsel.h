#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.22 2002-06-24 15:01:39 nanne Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
#include <multiid.h>
class IOObj;
class CtxtIOObj;
class IODirEntryList;
class uiLabeledListBox;
class uiGenInput;
class Translator;


/*! \brief dialog returning an IOObj* after successful go(). */

class uiIOObjRetDlg : public uiDialog
{
public:

			uiIOObjRetDlg(uiParent* p,const Setup& s)
			: uiDialog(p,s) {}

    virtual const IOObj* ioObj() const		= 0;
 
};


/*! \brief Dialog for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSelDlg : public uiIOObjRetDlg
{
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
				      const char* seltxt=0,bool multisel=false);
			~uiIOObjSelDlg();

    int			nrSel() const;
    const IOObj*	selected(int idx=0) const;
    const IOObj*	ioObj() const			{ return selected(0); }

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    void		setInitOutputName(const char*);

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;
    bool		ismultisel;
    bool		initoutpnm;

    uiLabeledListBox*	listfld;
    uiGenInput*		nmfld;
    uiGroup*		grp;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
    void		rightClk(CallBacker*);

    virtual bool	createEntry(const char*);
    bool		rmEntry(Translator*,bool);
};


/*! \brief UI element for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				   bool wthclear=false,
				   const char* selectionlabel=0);
			~uiIOObjSel();

    bool		commitInput(bool mknew);

    void		updateInput();	//!< updates from CtxtIOObj
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
    BufferString	seltxt;

    void		doObjSel(CallBacker*);

    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

    virtual void	newSelection(uiIOObjRetDlg*)		{}
    virtual uiIOObjRetDlg* mkDlg();
    virtual IOObj*	createEntry(const char*);

};


#endif
