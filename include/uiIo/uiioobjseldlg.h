#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2001
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiioobjselgrp.h"
#include "uidialog.h"

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
		Setup(const uiString& titletxt=uiString::empty())
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
    };

			uiIOObjSelDlg(uiParent*,const IOObjContext&);
			uiIOObjSelDlg(uiParent*,const CtxtIOObj&,
			const uiString& titletxt=uiString::empty());
			uiIOObjSelDlg(uiParent*,const Setup&,const CtxtIOObj&);
			~uiIOObjSelDlg();

    int			nrChosen() const	{ return selgrp_->nrChosen(); }
    DBKey		chosenID(int i=0) const { return selgrp_->chosenID(i); }
    void		getChosen( DBKeySet& ids ) const
						{ selgrp_->getChosen( ids ); }
    void		getChosen( BufferStringSet& nms ) const
						{ selgrp_->getChosen( nms ); }
    void		chooseAll( bool yn=true ) { selgrp_->chooseAll( yn ); }

    const IOObj*	ioObj() const;

    uiIOObjSelGrp*	selGrp()		{ return selgrp_; }
    bool		fillPar( IOPar& i ) const {return selgrp_->fillPar(i);}
    void		usePar( const IOPar& i ) { selgrp_->usePar(i); }

    void		setConfirmOverwrite(bool);
    void		setSurveyDefaultSubsel(const char*);

protected:

    void		statusMsgCB(CallBacker*);

    Setup		setup_;
    uiIOObjSelGrp*	selgrp_;

private:

    void		init(const CtxtIOObj&);
    static uiString	selTxt(bool forread);
    CtxtIOObj*		crctio_;

};
