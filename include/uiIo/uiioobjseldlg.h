#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "uiioobjselgrp.h"

/*!\brief Dialog letting the user select an object. It returns an IOObj* after
	  successful go(). */

mExpClass(uiIo) uiIOObjRetDlg : public uiDialog
{ mODTextTranslationClass(uiIOObjRetDlg);
public:
				~uiIOObjRetDlg();

    virtual const IOObj*	ioObj() const		= 0;
    virtual uiIOObjSelGrp*	selGrp()		{ return 0; }

protected:
				uiIOObjRetDlg(uiParent*,const Setup&);
};


mExpClass(uiIo) uiIOObjSelDlg : public uiIOObjRetDlg
{ mODTextTranslationClass(uiIOObjSelDlg);
public:

    mExpClass(uiIo) Setup
    {
    public:
		Setup(const uiString& titletxt=uiString::emptyString())
			    : titletext_(titletxt)
			    , multisel_(false)
			    , allowsetsurvdefault_(true)
			    , withwriteopts_(true)
			    , withinserters_(true)		{}
		virtual	~Setup()				{}

	mDefSetupMemb(uiString,titletext)
	mDefSetupMemb(bool,multisel)
	mDefSetupMemb(bool,allowsetsurvdefault)
	mDefSetupMemb(bool,withwriteopts)
	mDefSetupMemb(bool,withinserters)
	mDefSetupMemb(BufferStringSet,trsnotallwed);
    };

			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
			    const uiString& titletxt=uiString::emptyString());
			uiIOObjSelDlg(uiParent*,const Setup&,const CtxtIOObj&);
			~uiIOObjSelDlg();

    int			nrChosen() const	{ return selgrp_->nrChosen(); }
    const MultiID	chosenID(int i=0) const { return selgrp_->chosenID(i); }
    void		getChosen( TypeSet<MultiID>& ids ) const
						{ selgrp_->getChosen( ids ); }
    void		getChosen( BufferStringSet& nms ) const
						{ selgrp_->getChosen( nms ); }
    void		chooseAll( bool yn=true ) { selgrp_->chooseAll( yn ); }

    const IOObj*	ioObj() const override;

    uiIOObjSelGrp*	selGrp() override		{ return selgrp_; }
    bool		fillPar( IOPar& i ) const {return selgrp_->fillPar(i);}
    void		usePar( const IOPar& i ) { selgrp_->usePar(i); }

    void		setSurveyDefaultSubsel(const char*);
    bool		isForRead();

protected:

    bool		acceptOK(CallBacker*) override
			{ return selgrp_->updateCtxtIOObj(); }
    void		statusMsgCB(CallBacker*);

    Setup		setup_;
    uiIOObjSelGrp*	selgrp_					= nullptr;

private:

    void		init(const CtxtIOObj&);
    static uiString	selTxt(bool forread);

};
