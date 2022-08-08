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
#include "multiid.h"

class uiStepOutSel;
class uiSelZRange;
class uiWellParSel;


/*! \brief UI for WellPosProvider */

mExpClass(uiWell) uiWellPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiWellPosProvGroup);
public:
			uiWellPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiWellPosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		setExtractionDefaults() override;

    bool		getIDs(TypeSet<MultiID>&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiWellPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiWellParSel*	wellfld_;
    uiStepOutSel*	stepoutfld_;
    uiSelZRange*	zrgfld_;

};

