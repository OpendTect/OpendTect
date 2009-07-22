#ifndef uiisopachmaker_h
#define uiisopachmaker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
 RCS:           $Id: uiisopachmaker.h,v 1.3 2009-07-22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"
class uiIOObjSel;
class uiGenInput;
class CtxtIOObj;
class DataPointSet;
namespace EM 
{ 
    class Horizon3D; 
    class EMObject;
}


/*! \brief Create isopach as attribute of horizon */

class uiIsopachMaker : public uiDialog
{
public:

			uiIsopachMaker(uiParent*,EM::ObjectID);
			~uiIsopachMaker();

    const char*		attrName() const	{ return attrnm_; }
    const DataPointSet&	getDPS()		{ return dps_; }

protected:

    uiIOObjSel*		basesel_;
    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    DataPointSet&	dps_;

    CtxtIOObj&		basectio_;
    CtxtIOObj&		ctio_;
    EM::ObjectID	horid_;
    BufferString	attrnm_;
    EM::EMObject*	baseemobj_;
    bool		saveattr_;
    
    bool		acceptOK(CallBacker*);
    BufferString	getHorNm(EM::ObjectID);
    void		toHorSel(CallBacker*);

    bool		doWork();

};


#endif
