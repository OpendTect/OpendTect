#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiposprovgroup.h"
#include "dbkey.h"

class uiStepOutSel;
class uiSelZRange;
class uiMultiWellSel;


/*! \brief UI for WellPosProvider */

mExpClass(uiWell) uiWellPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiWellPosProvGroup);
public:
			uiWellPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiWellPosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(uiString&) const;

    void		setExtractionDefaults();

    bool		getIDs(DBKeySet&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiWellPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiMultiWellSel*	wellfld_;
    uiStepOutSel*	stepoutfld_;
    uiSelZRange*	zrgfld_;

};
