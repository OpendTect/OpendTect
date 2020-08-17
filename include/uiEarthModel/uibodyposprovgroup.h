#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          Nov 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiposprovgroup.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
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

    virtual void		usePar(const IOPar&);
    virtual bool		fillPar(IOPar&) const;
    void			getSummary(BufferString&) const;

    bool			getID(MultiID&) const;

protected:

    void			ioChg(CallBacker*);
    CtxtIOObj&			ctio_;

    uiIOObjSel*			bodyfld_;
    uiGenInput*			inoutbut_;
    uiPosSubSel*                outsidergfld_;
};

