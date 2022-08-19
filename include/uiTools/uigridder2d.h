#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"
#include "factory.h"

class Gridder2D;
class InverseDistanceGridder2D;
class uiGenInput;


mExpClass(uiTools) uiGridder2DSel : public uiDlgGroup
{ mODTextTranslationClass(uiGridder2DSel);
public:
    				uiGridder2DSel(uiParent*,const Gridder2D*);
    				~uiGridder2DSel();

    const Gridder2D*		getSel();
    const char*			errMsg() const override;

protected:
    void			selChangeCB(CallBacker*);
    const Gridder2D*		original_;
    uiGenInput*			griddingsel_;

    ObjectSet<uiDlgGroup>	griddingparams_;
    ObjectSet<Gridder2D>	gridders_;
};

mExpClass(uiTools) uiInverseDistanceGridder2D : public uiDlgGroup
{ mODTextTranslationClass(uiInverseDistanceGridder2D)
public:
    static void		initClass();
    static uiDlgGroup*	create(uiParent*,Gridder2D*);

    			uiInverseDistanceGridder2D(uiParent*,
					InverseDistanceGridder2D&);

    bool		acceptOK() override;
    bool		rejectOK() override;
    bool		revertChanges() override;

    const char*		errMsg() const override;

protected:

    uiGenInput*			searchradiusfld_;
    const float			initialsearchradius_;

    InverseDistanceGridder2D&	idg_;
};


mDefineFactory2Param( uiTools, uiDlgGroup, uiParent*, Gridder2D*,
		      uiGridder2DFact );
