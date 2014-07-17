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
#include "uiiosel.h"
#include "uidialog.h"
#include "multiid.h"

class CtxtIOObj;
class IODirEntryList;
class IOObj;
class IOObjContext;
class Translator;
class uiGenInput;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiListBox;
class uiToolButton;
class uiListBoxChoiceIO;
class uiComboBox;

/*!\brief Dialog letting the user select an object. It returns an IOObj* after
	  successful go(). */

mExpClass(uiIo) uiIOObjRetDlg : public uiDialog
{ mODTextTranslationClass(uiIOObjRetDlg);
public:

			uiIOObjRetDlg(uiParent* p,const Setup& s)
			: uiDialog(p,s) {}

    virtual const IOObj*	ioObj() const		= 0;

    virtual uiIOObjSelGrp*	selGrp()		{ return 0; }
};


/*!\brief Basic group for letting the user select an object.

  For write, you always need to call updateCtxtIOObj(). as a new IOObj may need
  to be created. In any case, if you want to have the CtxtIOObj updated,
  updateCtxtIOObj() is required. Otherwise, this is not needed.

*/


mExpClass(uiIo) uiIOObjSelGrp : public uiGroup
{
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup( OD::ChoiceMode cm=OD::ChooseOnlyOne )
			    : choicemode_(cm)
			    , allowreloc_(false)
			    , allowremove_(true)
			    , allowsetdefault_(false)
			    , confirmoverwrite_(true)	{}

	mDefSetupMemb(OD::ChoiceMode,choicemode);
	mDefSetupMemb(bool,allowreloc);
	mDefSetupMemb(bool,allowremove);
	mDefSetupMemb(bool,allowsetdefault);
	mDefSetupMemb(bool,confirmoverwrite);

	inline bool	isMultiChoice() const
			{ return ::isMultiChoice( choicemode_ ); }

    };

#   define		mDefuiIOObjSelGrpConstructors(ctxtclss) \
			uiIOObjSelGrp(uiParent*,const ctxtclss&); \
			uiIOObjSelGrp(uiParent*,const ctxtclss&, \
					const uiString& seltxt); \
			uiIOObjSelGrp(uiParent*,const ctxtclss&, \
					const Setup&); \
			uiIOObjSelGrp(uiParent*,const ctxtclss&, \
					const uiString& seltxt,const Setup&)

			mDefuiIOObjSelGrpConstructors(IOObjContext);
			mDefuiIOObjSelGrpConstructors(CtxtIOObj);
			~uiIOObjSelGrp();

    bool		isEmpty() const;
    int			size() const;
    inline bool		isMultiChoice() const { return setup_.isMultiChoice(); }

			// mostly interesting for read
    int			currentItem() const;
    MultiID		currentID() const;
    int			nrChosen() const;
    bool		isChosen(int) const;
    const MultiID&	chosenID(int idx=0) const;
    void		getChosen(TypeSet<MultiID>&) const;
    void		getChosen(BufferStringSet&) const;
    void		setCurrent(int);
    void		setCurrent(const MultiID&);
    void		setChosen(int,bool yn=true);
    void		setChosen(const TypeSet<MultiID>&);
    void		chooseAll(bool yn=true);

    bool		updateCtxtIOObj(); //!< mostly interesting for write
    const CtxtIOObj&	getCtxtIOObj() const		{ return ctio_; }
    const IOObjContext&	getContext() const;
    void		setContext(const IOObjContext&);

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

    Notifier<uiIOObjSelGrp> selectionChanged;
    Notifier<uiIOObjSelGrp> itemChosen;
    Notifier<uiIOObjSelGrp> newStatusMsg;
				/*!< Triggers when there is a new message for
				     statusbars and similar */

    void		fullUpdate(const MultiID& kpselected);

protected:

    CtxtIOObj&		ctio_;
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
    uiListBoxChoiceIO*	lbchoiceio_;

    void		fullUpdate(int);
    void		fillListBox();
    IOObj*		getIOObj(int);
    virtual bool	createEntry(const char*);
    IOObj*		updStatusBarInfo(bool);
    void		triggerStatusMsg(const char*);

    void		setInitial(CallBacker*);
    void		selChg(CallBacker*);
    void		choiceChg(CallBacker*);
    void		filtChg(CallBacker*);
    void		delPress(CallBacker*);
    void		makeDefaultCB(CallBacker*);
    void		readChoiceDone(CallBacker*);
    void		writeChoiceReq(CallBacker*);

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

    int			nrChosen() const	{ return selgrp_->nrChosen(); }
    const MultiID&	chosenID(int i=0) const { return selgrp_->chosenID(i); }
    void		getChosen( TypeSet<MultiID>& ids ) const
						{ selgrp_->getChosen( ids ); }
    void		getChosen( BufferStringSet& nms ) const
						{ selgrp_->getChosen( nms ); }
    void		chooseAll( bool yn=true ) { selgrp_->chooseAll( yn ); }

    const IOObj*	ioObj() const;

    uiIOObjSelGrp*	selGrp()		{ return selgrp_; }
    bool		fillPar( IOPar& i ) const {return selgrp_->fillPar(i);}
    void		usePar( const IOPar& i ) { selgrp_->usePar(i); }

    void		setSurveyDefaultSubsel(const char*);

protected:

    bool		acceptOK(CallBacker*)
			{ return selgrp_->updateCtxtIOObj(); }
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
			Setup( const uiString& seltext=0 )
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

    virtual void	updateInput();	//!< a.o. updates from CtxtIOObj
    virtual void	processInput(); //!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    virtual bool	existingTyped() const
					{ return existingUsrName(getInput()); }
					//!< returns false is typed input is
					//!< not an existing IOObj name

    virtual MultiID	validKey() const; //!< no side-effects
    virtual uiObject*	endObj(bool left);

protected:

    uiComboBox*		wrtrselfld_;

    CtxtIOObj&		inctio_;
    CtxtIOObj&		workctio_;
    Setup		setup_;
    HelpKey		helpkey_;
    bool		inctiomine_;
    ObjectSet<const Translator> wrtrs_;

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

