#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.28 2003-05-12 16:15:23 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
#include <multiid.h>
class IOObj;
class IOStream;
class CtxtIOObj;
class uiGenInput;
class Translator;
class IODirEntryList;
class uiLabeledListBox;
class uiToolButton;


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

    uiLabeledListBox*	listfld;
    uiGenInput*		nmfld;
    uiGroup*		grp;
    uiToolButton*	locbut;
    uiToolButton*	robut;
    uiToolButton*	renbut;
    uiToolButton*	rembut;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
    void		tbPush(CallBacker*);

    virtual bool	createEntry(const char*);

    bool		rmEntry(bool);
    bool		renEntry(Translator*);
    bool		chgEntry(Translator*);
    bool		roEntry(Translator*);
    bool		renImpl(Translator*,IOStream&,IOStream&);

};


//! Removes the underlying file(s) that an IOObj describes, with warnings
//!< if exist_lbl set to true, " existing " is added to message.
bool uiRmIOObjImpl(IOObj&,bool exist_lbl=false);

/*! \brief UI element for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSel : public uiIOSelect
{
public:
			uiIOObjSel(uiParent*,CtxtIOObj&,const char* txt=0,
				   bool wthclear=false,
				   const char* selectionlabel=0,
				   const char* buttontxt="Select ...");
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
