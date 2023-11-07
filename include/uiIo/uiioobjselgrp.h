#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uigroup.h"
#include "bufstringset.h"
#include "multiid.h"
#include "odcommonenums.h"


class CtxtIOObj;
class FileSystemWatcher;
class IOObj;
class IOObjContext;
class Translator;

class uiButton;
class uiComboBox;
class uiGenInput;
class uiIOObjInserter;
class uiIOObjManipGroup;
class uiIOObjSelGrp;
class uiIOObjSelGrpManipSubj;
class uiIOObjSelWriteTranslator;
class uiListBox;
class uiListBoxChoiceIO;
class uiToolButton;


/*!\brief Basic group for letting the user select an object.

  For write, you always need to call updateCtxtIOObj(). as a new IOObj may need
  to be created. In any case, if you want to have the CtxtIOObj updated,
  updateCtxtIOObj() is required. Otherwise, this is not needed.

*/


mExpClass(uiIo) EntryData
{
public:
			EntryData(const MultiID&);
			EntryData(const MultiID&,const char* objnm,
				    const char* dispnm,
				    const char* icnnm,bool isdef);
			~EntryData();

    void		setIconName(const char*);
    void		setDisplayName(const char*);
    void		setObjName(const char*);
    void		setIsDef(bool isdef)	{ isdef_ = isdef; }

    const MultiID&	getMID() const		{ return mid_; }
    const char*		getDispNm() const	{ return dispnm_.buf(); }
    const char*		getObjNm() const	{ return objnm_.buf(); }
    const char*		getIcnNm() const	{ return icnnm_.buf(); }
    bool		isDef() const		{ return isdef_; }

protected:
    MultiID		mid_		= MultiID::udf();
    BufferString	icnnm_		= "empty";
    BufferString	dispnm_		= "NONE";
    BufferString	objnm_		= "NONE";
    bool		isdef_		= false;
};


mExpClass(uiIo) EntryDataSet : public ManagedObjectSet<EntryData>
{
public:
				EntryDataSet();
				~EntryDataSet();

    const EntryData*		getDataFor(const MultiID&) const;
    EntryData*			getDataFor(const MultiID&);
    EntryDataSet&		add(const MultiID&,bool isdef=false);
    EntryDataSet&		add(const MultiID&,const BufferString&,
				    const BufferString&,bool isdef=false);
    EntryDataSet&		removeMID(const MultiID&);
    EntryDataSet&		updateMID(const MultiID&, EntryData*);

    const TypeSet<MultiID>&	getIOObjIds() const;
    const TypeSet<int>&		getDefaultIdxs(bool reread=false) const;
    BufferStringSet		getIOObjNms() const;
    BufferStringSet		getDispNms() const;
    BufferStringSet		getIconNms() const;
    int				indexOfMID(const MultiID& mid) const;
    int				indexOfNm(const BufferString&,
					  bool isdispnm) const;

    void			erase() override;

protected:

    TypeSet<MultiID>		livemids_;
    mutable TypeSet<int>	defaultidxs_;
};


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
			    , confirmoverwrite_(true) {}
			~Setup()	{}

	mDefSetupMemb(OD::ChoiceMode,choicemode);
	mDefSetupMemb(bool,allowreloc);
	mDefSetupMemb(bool,allowremove);
	mDefSetupMemb(bool,allowsetdefault);
	mDefSetupMemb(bool,withinserters);
	mDefSetupMemb(bool,withwriteopts);
	mDefSetupMemb(bool,confirmoverwrite);
	mDefSetupMemb(BufferString,withctxtfilter);
	mDefSetupMemb(BufferStringSet,trsnotallwed);
	//!<key can be either a translator group name or omf metadata key

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
    MultiID		chosenID(int idx=0) const;
    void		getChosen(TypeSet<MultiID>&) const;
    void		getChosen(BufferStringSet&) const;
    void		setCurrent(int);
    void		setCurrent(const MultiID&);
    void		setChosen(int,bool yn=true);
    void		setChosen(const TypeSet<MultiID>&);
    void		chooseAll(bool yn=true);
    const TypeSet<MultiID>&	getIOObjIds() const;

    bool		updateCtxtIOObj(); //!< mostly interesting for write
    const CtxtIOObj&	getCtxtIOObj() const		{ return ctio_; }
    const IOObjContext& getContext() const;
    void		setContext(const IOObjContext&);
    void		setDefTranslator(const Translator*);

    uiGroup*		getTopGroup()			{ return topgrp_; }
    uiGenInput*		getNameField()			{ return nmfld_; }
    uiObject*		getFilterFieldAttachObj();
    uiListBox*		getListField()			{ return listfld_; }
    uiIOObjManipGroup*	getManipGroup();
    void		displayManipGroup(bool yn,bool shrink=false);

    void		setConfirmOverwrite( bool yn )
				{ setup_.confirmoverwrite_ = yn; }
    void		setAskedToOverwrite( bool yn )
				{ asked2overwrite_ = yn; }
    bool		askedToOverwrite() const { return asked2overwrite_; }

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Notifier<uiIOObjSelGrp> selectionChanged;
    Notifier<uiIOObjSelGrp> itemChosen;
    Notifier<uiIOObjSelGrp> newStatusMsg;
				/*!< Triggers when there is a new message for
				     statusbars and similar */
    Notifier<uiIOObjSelGrp> listUpdated;

    CNotifier<uiIOObjSelGrp,const MultiID&> itemAdded;
    CNotifier<uiIOObjSelGrp,const MultiID&> itemRemoved;
    CNotifier<uiIOObjSelGrp,const BufferStringSet&> itemChanged;

    void		fullUpdate(const MultiID& kpselected);
    void		fullUpdate(int);
    void		addEntry(const MultiID&);
    void		removeEntry(const MultiID&);
    void		updateEntry(const MultiID&,const BufferString& objnm,
				    const BufferString& dispnm,
				    const BufferString& icnnm);

protected:

    CtxtIOObj&			ctio_;
    Setup			setup_;
    EntryDataSet		dataset_;
    bool			asked2overwrite_		= false;

    uiListBox*			listfld_;
    uiGenInput*			nmfld_				= nullptr;
    uiGenInput*			filtfld_;
    uiIOObjSelGrpManipSubj*	manipgrpsubj_		= nullptr;
    uiIOObjSelWriteTranslator*	wrtrselfld_		= nullptr;
    uiToolButton*		mkdefbut_			= nullptr;
    uiListBoxChoiceIO*		lbchoiceio_;
    ObjectSet<uiButton>		insertbuts_;
    ObjectSet<uiIOObjInserter>	inserters_;
    uiGroup*			topgrp_;
    uiComboBox*			ctxtfiltfld_			= nullptr;
    FileSystemWatcher*		fswatcher_			= nullptr;

    void		fillListBox();
    void		addEntryToListBox(const MultiID&);
    IOObj*		getIOObj(int);
    virtual bool	createEntry(const char*);
    IOObj*		updStatusBarInfo(bool);
    void		triggerStatusMsg(const char*);

    void		initGrpCB(CallBacker*);
    void		setInitial(CallBacker*);
    void		selChg(CallBacker*);
    void		choiceChg(CallBacker*);
    void		filtChg(CallBacker*);
    void		objInserted(CallBacker*);
    void		objRemoved(CallBacker*);
    void		nameAvCB(CallBacker*);
    void		delPress(CallBacker*);
    void		makeDefaultCB(CallBacker*);
    void		readChoiceDone(CallBacker*);
    void		writeChoiceReq(CallBacker*);
    void		ctxtChgCB(CallBacker*);
    void		omfChgCB(CallBacker*);

private:

    void		init(const uiString& st=uiString::emptyString());
    void		mkTopFlds(const uiString&);
    void		mkWriteFlds();
    void		mkManipulators();

    void		newOutputNameCB(CallBacker*);

    friend class	uiIOObjSelDlg;
    friend class	uiIOObjSelGrpManipSubj;

};
