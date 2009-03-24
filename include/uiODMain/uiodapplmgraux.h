#ifndef uiodapplmgraux_h
#define uiodapplmgraux_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2009
 RCS:           $Id: uiodapplmgraux.h,v 1.1 2009-03-24 15:51:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiapplserv.h"
class uiODApplMgr;
class CtxtIOObj;
class uiVelSel;
class VelocityStretcher;
class ZAxisTransform;


/*!\brief uiApplService for OD */

mClass uiODApplService : public uiApplService
{
public:
    			uiODApplService( uiParent* p, uiODApplMgr& am )
			    : par_(p), applman_(am)	{}
    uiParent*		parent() const			{ return par_; }
    bool		eventOccurred(const uiApplPartServer*,int);
    void*		getObject(const uiApplPartServer*, int);

    uiODApplMgr&	applman_;
    uiParent*		par_;
};


/*!\brief Velocity model for depth scene */

mClass uiODApplMgrVelSel : public uiDialog
{
public:

			uiODApplMgrVelSel(uiParent*);
			~uiODApplMgrVelSel();

    bool		acceptOK(CallBacker*);
    ZAxisTransform*	transform();
    float		zScale() const		{ return zscale_; }
			
    CtxtIOObj&		ctio_;
    uiVelSel*		velsel_;
    VelocityStretcher*	trans_;
    float		zscale_;

};


#endif
