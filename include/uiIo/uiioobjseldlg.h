#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
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

			uiIOObjRetDlg(uiParent* p,const Setup& s)
			: uiDialog(p,s) {}

    virtual const IOObj*	ioObj() const		= 0;

    virtual uiIOObjSelGrp*	selGrp()		{ return 0; }
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

    int			nrChosen() const	{ return selgrp_->nrChosen(); }
    const MultiID	chosenID(int i=0) const { return selgrp_->chosenID(i); }
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
    bool		isForRead();

protected:

    bool		acceptOK(CallBacker*)
			{ return selgrp_->updateCtxtIOObj(); }
    void		statusMsgCB(CallBacker*);

    Setup		setup_;
    uiIOObjSelGrp*	selgrp_;

private:

    void		init(const CtxtIOObj&);
    static uiString	selTxt(bool forread);

};


