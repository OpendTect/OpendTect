#ifndef uidpsdemo_h
#define uidpsdemo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
 RCS:           $Id: uidpsdemo.h,v 1.3 2009-11-04 13:43:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class uiSeisSel;
class TaskRunner;
class uiIOObjSel;
class uiGenInput;
class DataPointSet;
namespace EM { class Horizon3D; }


/*!\brief Show a few uses of (ui)DataPointSet.

  Case is: extract amplitudes and frequencies along a horizon.
 
 */

mClass uiDPSDemo : public uiDialog
{ 	
public:

			uiDPSDemo(uiParent*);
			~uiDPSDemo();

protected:

    uiIOObjSel*		horfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		nrptsfld_;

    DataPointSet&	dps_;

    bool		acceptOK(CallBacker*);
    bool		doWork(const IOObj&,const IOObj&,int);

    bool		getRandPositions(const EM::Horizon3D&,int);
    bool		getSeisData(const IOObj&,TaskRunner&);
};


#endif
