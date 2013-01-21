#ifndef uidpsdemo_h
#define uidpsdemo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidpsdemomod.h"
#include "uidialog.h"
class IOObj;
class uiSeisSel;
class TaskRunner;
class uiIOObjSel;
class uiGenInput;
class DataPointSet;
class DataPointSetDisplayMgr;
namespace EM { class Horizon3D; }


/*!\brief Show a few uses of (ui)DataPointSet.

  Case is: extract amplitudes and frequencies along a horizon.
 
 */

mExpClass(uiDPSDemo) uiDPSDemo : public uiDialog
{ 	
public:

			uiDPSDemo(uiParent*,DataPointSetDisplayMgr* mgr=0);
			~uiDPSDemo();

protected:

    DataPointSet*	dps_;
    DataPointSetDisplayMgr* dpsdispmgr_;
    uiIOObjSel*		horfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		nrptsfld_;

    bool		acceptOK(CallBacker*);
    void		showSelPtsCB(CallBacker*);
    void		removeSelPtsCB(CallBacker*);

    bool		doWork(const IOObj&,const IOObj&,int);
    bool		getRandPositions(const EM::Horizon3D&,int,
	    				 DataPointSet&);
    bool		getSeisData(const IOObj&,DataPointSet&,TaskRunner&);

};


#endif

