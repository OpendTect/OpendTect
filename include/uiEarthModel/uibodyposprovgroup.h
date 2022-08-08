#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          Nov 2011
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiposprovgroup.h"

class uiBodySel;
class uiGenInput;
class uiPosSubSel;


/*! \brief UI for BodyPosProvider */
mExpClass(uiEarthModel) uiBodyPosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiBodyPosProvGroup);
public:
				uiBodyPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
				~uiBodyPosProvGroup();

    static void			initClass();
    static uiPosProvGroup*	create(uiParent*,const uiPosProvGroup::Setup&);

    void			usePar(const IOPar&) override;
    bool			fillPar(IOPar&) const override;
    void			getSummary(BufferString&) const override;

    bool			getID(MultiID&) const;

protected:

    void			ioChg(CallBacker*);

    uiBodySel*			bodyfld_;
    uiGenInput*			inoutbut_;
    uiPosSubSel*		outsidergfld_;
};

