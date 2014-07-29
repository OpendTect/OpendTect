#ifndef uiioobjselgrp_h
#define uiioobjselgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "multiid.h"
#include "bufstringset.h"

class IOObj;
class CtxtIOObj;
class Translator;
class IOObjContext;
class uiGenInput;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiListBox;
class uiToolButton;
class uiListBoxChoiceIO;
class uiIOObjSelWriteTranslator;


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
    const IOObjContext& getContext() const;
    void		setContext(const IOObjContext&);
    void		setDefTranslator(const Translator*);

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

    uiListBox*		listfld_;
    uiGenInput*		nmfld_;
    uiGenInput*		filtfld_;
    uiIOObjSelGrpManipSubj* manipgrpsubj;
    uiIOObjSelWriteTranslator* wrtrselfld_;
    uiToolButton*	mkdefbut_;
    uiListBoxChoiceIO*	lbchoiceio_;
    uiGroup*		topgrp_;

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


#endif
