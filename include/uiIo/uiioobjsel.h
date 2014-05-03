#ifndef uiioobjsel_h
#define uiioobjsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uiiosel.h"
#include "ctxtioobj.h"
#include "multiid.h"

class IODirEntryList;
class IOObj;
class uiGenInput;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiListBox;
class uiToolButton;

/*!
\brief Dialog letting the user select an object. It returns an IOObj* after
successful go().
*/

mExpClass(uiIo) uiIOObjRetDlg : public uiDialog
{
public:

			uiIOObjRetDlg(uiParent* p,const Setup& s)
			: uiDialog(p,s) {}

    virtual const IOObj*	ioObj() const		= 0;

    virtual uiIOObjSelGrp*	selGrp()		{ return 0; }
};


/*!\brief Basic group for letting the user select an object.

  For the multi-select selection modes the list will be checkable. The SelMode
  'AnyNumber' will allow the user to not check any item. In the 'AtLeastOne'
  mode the current item is used if the user doesn't check anything.
  In all cases 0 selected is possible (e.g. the list can be empty) - it's up
  to you whether this will be an error.

*/

mExpClass(uiIo) uiIOObjSelGrp : public uiGroup
{
public:

    enum SelMode	{ Single, AnyNumber, AtLeastOne };

    mExpClass(uiIo) Setup
    {
    public:
			Setup( SelMode sm=Single )
			    : selmode_(sm)
			    , allowreloc_(false)
			    , allowremove_(true)
			    , allowsetdefault_(false)
			    , confirmoverwrite_(true)	{}

	mDefSetupMemb(SelMode,selmode);
	mDefSetupMemb(bool,allowreloc);
	mDefSetupMemb(bool,allowremove);
	mDefSetupMemb(bool,allowsetdefault);
	mDefSetupMemb(bool,confirmoverwrite);

	bool		isMultiSel() const	{ return selmode_ != Single; }

    };
			uiIOObjSelGrp(uiParent*,const CtxtIOObj&);
			uiIOObjSelGrp(uiParent*,const CtxtIOObj&,
					const uiString& seltxt);
			uiIOObjSelGrp(uiParent*,const CtxtIOObj&,const Setup&);
			uiIOObjSelGrp(uiParent*,const CtxtIOObj&,
				      const uiString& seltxt,const Setup&);
			~uiIOObjSelGrp();
    bool		isEmpty() const;
    int			size() const;
    inline bool		isMultiSel() const	{ return setup_.isMultiSel(); }

    void		fullUpdate(const MultiID& kpselected);
    bool		processInput();
				/*!< has to be done before selected() can be
				   queried. It also creates an entry in IOM if
				   the selected object is new.  */

    int			nrSelected() const;
    bool		isSel(int) const;
    int			currentItem() const;
    MultiID		currentID() const;
    const MultiID&	selected(int idx=0) const;
				//!<\note processInput should be called first!
    void		setSelected(const TypeSet<MultiID>&);
    void		getSelected(TypeSet<MultiID>&) const;
    void		selectAll(bool yn=true);


    void		setContext(const IOObjContext&);
    const IOObjContext&	getCtxt() const			{ return ctio_.ctxt; }
    const CtxtIOObj&	getCtxtIOObj() const		{ return ctio_; }
    uiGroup*		getTopGroup()			{ return topgrp_; }
    uiGenInput*		getNameField()			{ return nmfld_; }
    uiListBox*		getListField()			{ return listfld_; }
    uiIOObjManipGroup*	getManipGroup();
    const ObjectSet<MultiID>& getIOObjIds() const	{ return ioobjids_; }
    void		setConfirmOverwrite( bool yn )
				{ setup_.confirmoverwrite_ = yn; }
    void		setAskedToOverwrite( bool yn )
				{ asked2overwrite_ = yn; }
    bool		askedToOverwrite() const { return asked2overwrite_; }
    void		setSurveyDefaultSubsel(const char* subsel);

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Notifier<uiIOObjSelGrp> selectionChg;
    Notifier<uiIOObjSelGrp> newStatusMsg;
				/*!< Triggers when there is a new message for
				     statusbars and similar */

protected:

    CtxtIOObj		ctio_;
    Setup		setup_;
    ObjectSet<MultiID>	ioobjids_;
    BufferStringSet	ioobjnms_;
    BufferStringSet	dispnms_;
    BufferString	surveydefaultsubsel_;
    bool		asked2overwrite_;

    uiIOObjSelGrpManipSubj* manipgrpsubj;
    uiListBox*		listfld_;
    uiGenInput*		nmfld_;
    uiGenInput*		filtfld_;
    uiGroup*		topgrp_;

    uiToolButton*	mkdefbut_;

    void		fullUpdate(int);
    void		newList();
    void		fillListBox();
    void		setCur(int);
    void		toStatusBar(const char*);
    IOObj*		getIOObj(int);
    virtual bool	createEntry(const char*);

    void		setInitial(CallBacker*);
    void		selChg(CallBacker*);
    void		filtChg(CallBacker*);
    void		delPress(CallBacker*);
    void		makeDefaultCB(CallBacker*);

private:

    void		init(const uiString& st=0);

    friend class	uiIOObjSelDlg;
    friend class	uiIOObjSelGrpManipSubj;

};


/*!
\brief Dialog for selection of IOObjs.
*/

mExpClass(uiIo) uiIOObjSelDlg : public uiIOObjRetDlg
{ mODTextTranslationClass(uiIOObjSelDlg);
public:
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
				      const uiString& seltxt=0,
				      bool multisel=false,
				      bool allowsetsurvdefault=false);

    int			nrSelected() const { return selgrp_->nrSelected(); }
    const MultiID&	selected( int i ) const	{ return selgrp_->selected(i); }
    const IOObj*	ioObj() const	{return selgrp_->getCtxtIOObj().ioobj;}
    uiIOObjSelGrp*	selGrp()		{ return selgrp_; }
    bool		fillPar( IOPar& i ) const {return selgrp_->fillPar(i);}
    void		usePar( const IOPar& i ) { selgrp_->usePar(i); }

    void		setSurveyDefaultSubsel(const char*);

protected:

    bool		acceptOK(CallBacker*){return selgrp_->processInput();}
    void		statusMsgCB(CallBacker*);

    uiIOObjSelGrp*	selgrp_;
};


/*!
\brief User Interface (UI) element for selection of IOObjs.

  User gets the possibility to select an object of a certain type.

  If nothing is selected, an error will be generated if setup.mandatory_ is
  true. This is the default. Thus, you can simply do, in acceptOK():
  const IOObj* theobj = theselfld_->ioobj();
  if ( !theobj ) return false;
*/

mExpClass(uiIo) uiIOObjSel : public uiIOSelect
{ mODTextTranslationClass(uiIOObjSel);
public:

    mExpClass(uiIo) Setup : public uiIOSelect::Setup
    {
    public:
			Setup( const char* seltext=0 )
			    : uiIOSelect::Setup(seltext)
			    , confirmoverwr_(true)
			    , filldef_(true)		{}

	mDefSetupMemb(bool,confirmoverwr)
	mDefSetupMemb(bool,filldef)	//!< only if forread and !ctio.ioobj
    };

			uiIOObjSel(uiParent*,const IOObjContext&,
					const char* seltxt=0);
			uiIOObjSel(uiParent*,const IOObjContext&,const Setup&);
			~uiIOObjSel();

    void		setInput(const IOObj&);
    void		setInput(const MultiID&);

    MultiID		key(bool noerr=false) const;
    const IOObj*	ioobj(bool noerr=false) const;
    IOObj*		getIOObj(bool noerr=false); //!< My IOObj becomes yours

    virtual bool	fillPar(IOPar&) const;
    bool		fillPar(IOPar&,const char* baseky) const;
    virtual void	usePar(const IOPar&);
    void		usePar(const IOPar&,const char* baseky);

    void		setForRead(bool);
    void		setConfirmOverwrite( bool yn )
					{ setup_.confirmoverwr_ = yn; }
    void		setHelpKey(const HelpKey& helpkey) { helpkey_=helpkey; }

    virtual void	updateInput();	//!< updates from CtxtIOObj
    virtual void	processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    virtual bool	existingTyped() const
					{ return existingUsrName(getInput()); }
					//!< returns false is typed input is
					//!< not an existing IOObj name

    virtual MultiID	validKey() const; //!< no side-effects

protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		workctio_;
    Setup		setup_;
    HelpKey		helpkey_;
    bool		inctiomine_;

    void		preFinaliseCB(CallBacker*);
    void		doObjSel(CallBacker*);

    virtual const char*	userNameFromKey(const char*) const;
    virtual void	objSel();
    virtual void	commitSucceeded()			{}

    virtual void	fillDefault();
    virtual void	newSelection(uiIOObjRetDlg*)		{}
    virtual uiIOObjRetDlg* mkDlg();
    virtual IOObj*	createEntry(const char*);
    void		obtainIOObj();
    bool		existingUsrName(const char*) const;
    void		doCommit(bool) const;


public: // old style

    /* Comments for old style only:
On creation, the object makes a copy of your CtxtIOObj. When needed, you can do
commitInput() to get the selection in your input CtxtIOObj;

You *have* to do commitInput() to get any selection! Other functions like
processInput() are special stuff for special situations.

You have to check commitInput() and issue and error message if necessary. In
the new style, this is done if the setup.optional_ flag is false (this is the
default).
*/

			uiIOObjSel(uiParent*,CtxtIOObj&,const char* seltxt=0);
			uiIOObjSel(uiParent*,CtxtIOObj&,const Setup&);
    bool		commitInput();
    bool		doCommitInput(bool&);
    CtxtIOObj&		ctxtIOObj( bool work=false )
					{ return work ? workctio_ : inctio_; }

};


#endif

