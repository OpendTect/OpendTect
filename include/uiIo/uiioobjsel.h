#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.36 2004-10-05 15:26:20 bert Exp $
________________________________________________________________________

-*/

#include <uiiosel.h>
#include <uidialog.h>
#include <multiid.h>
class IOObj;
class IOStream;
class CtxtIOObj;
class uiGenInput;
class IODirEntryList;
class uiLabeledListBox;
class uiIOObjManipGroup;


/*! \brief Dialog letting the user select an object.
           It returns an IOObj* after successful go(). */

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

protected:

    const CtxtIOObj&	ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;
    bool		ismultisel;

    uiIOObjManipGroup*	manipgrp;
    uiLabeledListBox*	listfld;
    uiGenInput*		nmfld;
    uiGroup*		topgrp;

    bool		acceptOK(CallBacker*);
    void		selChg(CallBacker*);
    void		preReloc(CallBacker*);

    void		replaceFinaliseCB(const CallBack&);

    virtual bool	createEntry(const char*);

};


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

    virtual void	updateInput();	//!< updates from CtxtIOObj
    virtual void	processInput(); //!< Match user typing with existing
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
    void		obtainIOObj();

};


/*!\mainpage User Interface related to I/O

  This module contains some basic classes that handle the selection and
  management of an IOObj. An IOObj contains all info necessary to be able
  to load or store an object from disk. In OpendTect, users select IOObj's,
  not files (at least most often not direct but via na IOObj entry). Users
  recognise the IOObj by its name. Every IOObj has a unique identifier, the
  IOObj's key() which is a MultiID.

  In order to make the right selection, the IOObj selectors must know the
  context of the selection: what type of object, is it for read or write,
  should the user be able to create a new entry, and so forth. That's why
  you have to pass a CtxtIOObj .

  Other objects have been stuffed into this module as there was space left.
  More seriously, one can say that those objects are too OpendTect specific for
  the uiTools directory, but too general for any specific UI directory.

*/

#endif
