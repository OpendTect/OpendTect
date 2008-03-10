#ifndef uisurfaceposprov_h
#define uisurfaceposprov_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uisurfaceposprov.h,v 1.3 2008-03-10 16:35:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposprovgroup.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiSpinBox;
class uiSelZRange;
class uiLabel;

/*! \brief UI for SurfacePosProvider */

class uiSurfacePosProvGroup : public uiPosProvGroup
{
public:
			uiSurfacePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiSurfacePosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(BufferString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiSurfacePosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio1_;
    CtxtIOObj&		ctio2_;
    const float		zfac_;

    uiIOObjSel*		surf1fld_;
    uiIOObjSel*		surf2fld_;
    uiGenInput*		issingfld_;
    uiSpinBox*		zstepfld_;
    uiLabel*		zsteplbl_;
    uiSelZRange*	extrazfld_;

    void		selChg(CallBacker*);
};


#endif
