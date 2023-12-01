#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uiiosel.h"
#include "helpview.h"

class IOObj;
class CtxtIOObj;
class IOObjContext;
class uiIOObjRetDlg;
class uiIOObjInserter;
class uiIOObjSelWriteTranslator;
namespace ZDomain { class Info; }


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
			Setup(const uiString& seltxt=uiString::empty())
			    : uiIOSelect::Setup(seltxt)
			    , confirmoverwr_(true)
			    , withinserters_(true)
			    , withwriteopts_(true)
			    , filldef_(true)
			{}
	virtual		~Setup()		{}

	mDefSetupMemb(bool,confirmoverwr)
	mDefSetupMemb(bool,withinserters) //!< only if forread
	mDefSetupMemb(bool,withwriteopts) //!< only if !forread
	mDefSetupMemb(bool,filldef)	//!< only if forread and !ctio.ioobj
	mDefSetupMemb(BufferString,withctxtfilter);
	mDefSetupMemb(BufferStringSet,trsnotallwed);

    };

			uiIOObjSel(uiParent*,const IOObjContext&,
			       const uiString& seltxt=uiString::empty());
			uiIOObjSel(uiParent*,const IOObjContext&,const Setup&);
			~uiIOObjSel();

    virtual void	setInput(const IOObj&);
    virtual void	setInput(const MultiID&);
    void		setEmpty() override;

    MultiID		key(bool noerr=false) const;
    const IOObj*	ioobj(bool noerr=false) const;
    IOObj*		getIOObj(bool noerr=false); //!< My IOObj becomes yours

    virtual bool	fillPar(IOPar&) const;
    bool		fillPar(IOPar&,const char* baseky) const;
    virtual void	usePar(const IOPar&);
    void		usePar(const IOPar&,const char* baseky);

    void		fillEntries(); //!< only if forread
    void		setConfirmOverwrite( bool yn )
					{ setup_.confirmoverwr_ = yn; }
    void		setHelpKey(const HelpKey& helpkey) { helpkey_=helpkey; }
    void		require(const char* key,const char* typ,
				bool allowempty=false);
    void		requireType(const char*,bool allowempty=false);
    void		requireZDomain(const ZDomain::Info&,
				       bool allowempty=true);
    const ZDomain::Info* requiredZDomain() const;

    virtual void	updateInput();	//!< a.o. updates from CtxtIOObj
    void		processInput() override;
					//!< Match user typing with existing
					//!< IOObjs, then set item accordingly
    virtual bool	existingTyped() const
					{ return existingUsrName(getInput()); }
					//!< returns false is typed input is
					//!< not an existing IOObj name

    virtual MultiID	validKey() const; //!< no side-effects
    uiObject*		endObj(bool left) override;

    static IOObjContext getWriteIOObjCtxt(IOObjContext);

protected:

    uiIOObjSelWriteTranslator* wrtrselfld_;

    CtxtIOObj&		inctio_;
    CtxtIOObj&		workctio_;
    Setup		setup_;
    HelpKey		helpkey_;
    bool		inctiomine_;
    ObjectSet<uiIOObjInserter> inserters_;

    void		preFinalizeCB(CallBacker*);
    void		doObjSel(CallBacker*);
    void		objInserted(CallBacker*);
    void		survChangedCB(CallBacker*);
    void		optCheckCB(CallBacker*);
    void		initRead();

    const char*		userNameFromKey(const char*) const override;
    void		objSel() override;
    virtual void	commitSucceeded()			{}

    virtual void	fillDefault();
    virtual void	newSelection(uiIOObjRetDlg*)		{}
    virtual uiIOObjRetDlg* mkDlg();
    virtual IOObj*	createEntry(const char*);
    void		obtainIOObj();
    bool		existingUsrName(const char*) const;
    void		doCommit(bool) const;

private:

    void		init();

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

			uiIOObjSel(uiParent*,CtxtIOObj&,
				const uiString& seltxt=uiString::empty());
			uiIOObjSel(uiParent*,CtxtIOObj&,const Setup&);
    bool		commitInput();
    bool		doCommitInput(bool&);
    CtxtIOObj&		ctxtIOObj( bool work=false )
					{ return work ? workctio_ : inctio_; }

};


#define mWriteIOObjContext(clss) \
    uiIOObjSel::getWriteIOObjCtxt(mIOObjContext(clss))
#define mRWIOObjContext(clss,forread) \
    (forread ? mIOObjContext(clss) : mWriteIOObjContext(clss))
