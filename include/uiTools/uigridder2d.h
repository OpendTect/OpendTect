#ifndef uigridder2d_h
#define uigridder2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"
#include "factory.h"

class Gridder2D;
class InverseDistanceGridder2D;
class uiGenInput;


mClass(uiTools) uiGridder2DSel : public uiDlgGroup
{
public:
    				uiGridder2DSel(uiParent*,const Gridder2D*);
    				~uiGridder2DSel();

    const Gridder2D*		getSel();

protected:
    void			selChangeCB(CallBacker*);
    const Gridder2D*		original_;
    uiGenInput*			griddingsel_;

    ObjectSet<uiDlgGroup>	griddingparams_;
    ObjectSet<Gridder2D>	gridders_;
};

mClass(uiTools) uiInverseDistanceGridder2D : public uiDlgGroup
{
public:
    static void		initClass();
    static uiDlgGroup*	create(uiParent*,Gridder2D*);

    			uiInverseDistanceGridder2D(uiParent*,
					InverseDistanceGridder2D&);

    bool		acceptOK();
    bool		rejectOK();
    bool		revertChanges();

    const char*		errMsg() const;

protected:

    uiGenInput*			searchradiusfld_;
    const float			initialsearchradius_;

    InverseDistanceGridder2D&	idg_;
};


mDefineFactory2Param( uiTools, uiDlgGroup, uiParent*, Gridder2D*,
		      uiGridder2DFact );


#endif

