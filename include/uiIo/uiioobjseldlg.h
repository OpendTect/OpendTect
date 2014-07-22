#ifndef uiioobjseldlg_h
#define uiioobjseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id$
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


#endif
