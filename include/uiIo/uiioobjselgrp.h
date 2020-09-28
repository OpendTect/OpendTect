#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"
#include "dbkey.h"
#include "bufstringset.h"


class IOObj;
class CtxtIOObj;
class Translator;
class IOObjContext;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiLineEdit;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiListBox;
class uiButton;
class uiToolButton;
class uiIOObjInserter;
class uiListBoxChoiceIO;
class uiIOObjSelWriteTranslator;


/*!\brief Basic group for letting the user select an object.

  For write, you always need to call updateCtxtIOObj(). as a new IOObj may need
  to be created. In any case, if you want to have the CtxtIOObj updated,
  updateCtxtIOObj() is required. Otherwise, this is not needed.

*/


mExpClass(uiIo) uiIOObjSelGrp : public uiGroup
{ mODTextTranslationClass(uiIOObjSelGrp);
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup( OD::ChoiceMode cm=OD::ChooseOnlyOne )
			    : choicemode_(cm)
			    , allowreloc_(false)
			    , allowremove_(true)
			    , allowsetdefault_(false)
			    , withinserters_(true)
			    , withwriteopts_(true)
			    , confirmoverwrite_(true)
			    , withsort_(false)
			    , withrefresh_(true)
			    , withctxtfilter_(nullptr)	{}

	mDefSetupMemb(OD::ChoiceMode,choicemode);
	mDefSetupMemb(bool,allowreloc);
	mDefSetupMemb(bool,allowremove);
	mDefSetupMemb(bool,allowsetdefault);
	mDefSetupMemb(bool,withinserters);
	mDefSetupMemb(bool,withwriteopts);
	mDefSetupMemb(bool,confirmoverwrite);
	mDefSetupMemb(bool,withsort);
	mDefSetupMemb(bool,withrefresh);
	mDefSetupMemb(const char*,withctxtfilter);

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
    DBKey		currentID() const;
    int			nrChosen() const;
    bool		isChosen(int) const;
    DBKey		chosenID(int idx=0) const;
    void		getChosen(DBKeySet&) const;
    void		getChosen(BufferStringSet&) const;
    void		setCurrent(int);
    void		setCurrent(const DBKey&);
    void		setChosen(int,bool yn=true);
    void		setChosen(const DBKeySet&);
    void		chooseAll(bool yn=true);

    bool		updateCtxtIOObj(); //!< mostly interesting for write
    const CtxtIOObj&	getCtxtIOObj() const		{ return ctio_; }
    const IOObjContext&	getContext() const;
    void		setContext(const IOObjContext&);
    void		setDefTranslator(const Translator*);

    uiGroup*		getTopGroup()		    { return topgrp_; }
    uiGenInput*		getNameField()		    { return nmfld_; }
    uiLineEdit*		getFilterField()	    { return filtfld_; }
    uiComboBox*		getCntxtFiltFld()	    { return ctxtfiltfld_;  }
    uiListBox*		getListField()		    { return listfld_; }
    uiIOObjManipGroup*	getManipGroup();
    void		displayManipGroup(bool yn=true,bool shrink=false);
    const DBKeySet&	getIOObjIds() const		{ return ioobjids_; }

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
    Notifier<uiIOObjSelGrp> itemChanged;
    Notifier<uiIOObjSelGrp> newStatusMsg;
				/*!< Triggers when there is a new message for
				     statusbars and similar */

    void		fullUpdate(const DBKey& kpselected);
    void		fullUpdate(int);

protected:

    CtxtIOObj&		ctio_;
    Setup		setup_;
    DBKeySet		ioobjids_;
    BufferStringSet	ioobjnms_;
    BufferStringSet	dispnms_;
    BufferStringSet	iconnms_;
    TypeSet<od_int64>	modiftimes_;
    BufferString	surveydefaultsubsel_;
    bool		asked2overwrite_;
    bool		didtsort_ = false;

    uiListBox*		listfld_;
    uiGenInput*		nmfld_;
    uiLineEdit*		filtfld_;
    uiCheckBox*		tsortbox_ = nullptr;
    uiIOObjSelGrpManipSubj* manipgrpsubj;
    uiIOObjSelWriteTranslator* wrtrselfld_;
    uiToolButton*	mkdefbut_;
    uiListBoxChoiceIO*	lbchoiceio_;
    ObjectSet<uiButton>	insertbuts_;
    ObjectSet<uiIOObjInserter> inserters_;
    uiGroup*		topgrp_;
    uiComboBox*		ctxtfiltfld_ = nullptr;

    bool		doTimeSort() const;
    void		addModifTime(const IOObj&);
    void		fillListBox();
    IOObj*		getIOObj(int);
    virtual bool	createEntry(const char*);
    IOObj*		updStatusBarInfo(bool);
    void		triggerStatusMsg(const char*);

    void		setInitial(CallBacker*);
    void		itemChg(CallBacker*);
    void		selChg(CallBacker*);
    void		choiceChg(CallBacker*);
    void		refreshCB(CallBacker*);
    void		sortChgCB(CallBacker*);
    void		orderChgCB(CallBacker*);
    void		objInserted(CallBacker*);
    void		nameAvCB(CallBacker*);
    void		delPress(CallBacker*);
    void		makeDefaultCB(CallBacker*);
    void		readChoiceDone(CallBacker*);
    void		writeChoiceReq(CallBacker*);
    void		ctxtChgCB(CallBacker*);

private:

    void		init(const uiString& st=uiString::empty());
    void		mkTopFlds(const uiString&);
    void		mkWriteFlds();
    void		mkManipulators();

    void		newOutputNameCB(CallBacker*);

    friend class	uiIOObjSelDlg;
    friend class	uiIOObjSelGrpManipSubj;

};
