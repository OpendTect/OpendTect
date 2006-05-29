#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.45 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiiosel.h"
#include "uidialog.h"
#include "multiid.h"
class IOObj;
class IOStream;
class CtxtIOObj;
class uiGenInput;
class uiIOObjSelGrp;
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

    virtual const IOObj*	ioObj() const		= 0;
 
    virtual uiIOObjSelGrp*	selGrp()		{ return 0; }
};


/*! \brief Basic group for letting the user select an object. It 
	   can be used standalone in a dialog, or as a part of dialogs. */

class uiIOObjSelGrp : public uiGroup
{
public:
				uiIOObjSelGrp( uiParent*, const CtxtIOObj& ctio,
					       const char* seltxt=0,
					       bool multisel=false );
				~uiIOObjSelGrp();

    void			removeItem(const MultiID&);

    bool			processInput();
    				/*!< Processes the current selection so
				     selected() can be queried. It also creates
				     an entry in IOM if the selected object is
				     new.  */

    int				nrSel() const;
    const IOObj*		selected(int idx=0) const;
    				/*!<\note that processInput should be called
				          after selection, but before any call
					  to this.  */
    Notifier<uiIOObjSelGrp>	newstatusmessage;
    				/*!< Triggers when there is a new message for
				     statusbars and similar */
    const char*			statusmessage;

    void			selectionChange(CallBacker* = 0);
    				/*!< Updates the object when the selection has
				     been changed.  */

    void			setContext( const CtxtIOObj& );
    const CtxtIOObj&		getContext() const	{ return ctio; }
    uiGroup*			getTopGroup()		{ return topgrp; }
    uiGenInput*			getNameField()		{ return nmfld; }
    uiLabeledListBox*		getListField()		{ return listfld; }

    virtual bool		fillPar(IOPar&) const;
    virtual void		usePar(const IOPar&);


protected:
    CtxtIOObj&		ctio;
    IODirEntryList*	entrylist;
    IOObj*		ioobj;
    bool		ismultisel;

    uiIOObjManipGroup*	manipgrp;
    uiLabeledListBox*	listfld;
    uiGenInput*		nmfld;
    uiGenInput*		filtfld;
    uiGroup*		topgrp;

    void		preReloc( CallBacker* );

    bool		createEntry( const char* );
    void		fillList();
    void		rebuildList(CallBacker* = 0 );
    void		toStatusBar( const char* );
};

/*! \brief Dialog for selection of IOObjs

This class may be subclassed to make selection more specific.

*/

class uiIOObjSelDlg : public uiIOObjRetDlg
{
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
				      const char* seltxt=0,bool multisel=false);

    int			nrSel() const		{ return selgrp->nrSel(); }
    const IOObj*	selected(int i) const	{ return selgrp->selected(i); }
    const IOObj*	ioObj() const		{ return selgrp->selected(0); }

    virtual void	fillPar(IOPar& p) const	{ selgrp->fillPar(p); }
    virtual void	usePar(const IOPar& p)	{ selgrp->usePar(p); }

    uiIOObjSelGrp*	selGrp()		{ return selgrp; }

protected:

    bool		acceptOK(CallBacker*)	{return selgrp->processInput();}
    void		statusMsgCB(CallBacker*);
    void		setInitial(CallBacker*);

    uiIOObjSelGrp*	selgrp;
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
				   const char* buttontxt="&Select", 
				   bool keepmytxt=false);
			~uiIOObjSel();

    bool		commitInput(bool mknew);

    virtual void	updateInput();	//!< updates from CtxtIOObj
    virtual void	processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    virtual bool	existingTyped() const
					{ return existingUsrName(getInput()); }
					//!< returns false is typed input is
					//!< not an existing IOObj name
    CtxtIOObj&		ctxtIOObj()		{ return ctio; }

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		setUnselectables( const ObjectSet<MultiID>& s )
			{ deepCopy( unselabls, s ); }

protected:

    CtxtIOObj&		ctio;
    bool		forread;
    BufferString	seltxt;
    ObjectSet<MultiID>	unselabls;

    void		doObjSel(CallBacker*);

    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();

    virtual void	newSelection(uiIOObjRetDlg*)		{}
    virtual uiIOObjRetDlg* mkDlg();
    virtual IOObj*	createEntry(const char*);
    void		obtainIOObj();
    bool		existingUsrName(const char*) const;

};


#endif
