#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiioobjsel.h,v 1.55 2008-05-30 07:34:17 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiiosel.h"
#include "ctxtioobj.h"
#include "multiid.h"

class IODirEntryList;
class IOObj;
class IOStream;
class uiGenInput;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiListBox;


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
				uiIOObjSelGrp(uiParent*,const CtxtIOObj& ctio,
					      const char* seltxt=0,
					      bool multisel=false);
				~uiIOObjSelGrp();

    void			fullUpdate(const MultiID& kpselected);
    bool			processInput();
    				/*!< Processes the current selection so
				     selected() can be queried. It also creates
				     an entry in IOM if the selected object is
				     new.  */

    int				nrSel() const;
    const MultiID&		selected(int idx=0) const;
    				/*!<\note that processInput should be called
				          after selection, but before any call
					  to this.  */
    Notifier<uiIOObjSelGrp>	selectionChg;
    Notifier<uiIOObjSelGrp>	newStatusMsg;
    				/*!< Triggers when there is a new message for
				     statusbars and similar */

    void			setContext(const IOObjContext&);
    const CtxtIOObj&		getCtxtIOObj() const	{ return ctio_; }
    uiGroup*			getTopGroup()		{ return topgrp; }
    uiGenInput*			getNameField()		{ return nmfld; }
    uiListBox*			getListField()		{ return listfld; }
    uiIOObjManipGroup*		getManipGroup();
    const ObjectSet<MultiID>&	getIOObjIds() const	{ return ioobjids_; }
    void			setConfirmOverwrite( bool yn )
				{ confirmoverwrite_ = yn; }
    void			setAskedToOverwrite( bool yn )
				{ asked2overwrite_ = yn; }
    bool			askedToOverwrite() const
    				{ return asked2overwrite_; }

    virtual bool		fillPar(IOPar&) const;
    virtual void		usePar(const IOPar&);

protected:

    CtxtIOObj		ctio_;
    ObjectSet<MultiID>	ioobjids_;
    BufferStringSet	ioobjnms_;
    bool		ismultisel_;
    bool		confirmoverwrite_;
    bool		asked2overwrite_;

    friend class	uiIOObjSelDlg;
    friend class	uiIOObjSelGrpManipSubj;
    uiIOObjSelGrpManipSubj* manipgrpsubj;
    uiListBox*		listfld;
    uiGenInput*		nmfld;
    uiGenInput*		filtfld;
    uiGroup*		topgrp;

    void		fullUpdate(int);
    void		newList();
    void		fillListBox();
    void		setCur(int);
    void		toStatusBar( const char* );
    bool		createEntry( const char* );

    void		selChg(CallBacker*);
    void		filtChg(CallBacker*);
    void		delPress(CallBacker*);
    IOObj*		getIOObj(int);
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
    const MultiID&	selected( int i ) const	{ return selgrp->selected(i); }
    const IOObj*	ioObj() const	{ return selgrp->getCtxtIOObj().ioobj; }

    // virtual void	fillPar(IOPar& p) const	{ selgrp->fillPar(p); }
    // virtual void	usePar(const IOPar& p)	{ selgrp->usePar(p); }

    uiIOObjSelGrp*	selGrp()		{ return selgrp; }
    bool		fillPar( IOPar& i ) const { return selgrp->fillPar(i); }
    void		usePar( const IOPar& i ) { selgrp->usePar(i); }

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
    CtxtIOObj&		ctxtIOObj()	{ return ctio; }

    virtual bool	fillPar(IOPar&,const char* compky=0) const;
    virtual void	usePar(const IOPar&,const char* compky=0);

    void		setForRead( bool yn )		{ forread = yn; }
    void		setUnselectables( const ObjectSet<MultiID>& s )
			{ deepCopy( unselabls, s ); }

    void		setHelpID( const char* id )	{ helpid_ = id; }

protected:

    CtxtIOObj&		ctio;
    bool		forread;
    BufferString	seltxt;
    ObjectSet<MultiID>	unselabls;
    BufferString	helpid_;

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
