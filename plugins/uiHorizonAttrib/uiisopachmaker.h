#ifndef uiisopachmaker_h
#define uiisopachmaker_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2008
 RCS:           $Id: uiisopachmaker.h,v 1.1 2008-06-03 08:47:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"
class uiIOObjSel;
class uiGenInput;
class CtxtIOObj;
class DataPointSet;
namespace EM { class Horizon3D; }


/*! \brief Create isopach as attribute of horizon */

class uiIsopachMaker : public uiDialog
{
public:

			uiIsopachMaker(uiParent*,EM::ObjectID);
			~uiIsopachMaker();

    const char*		attrName() const	{ return attrnm_; }
    const DataPointSet&	getDPS()		{ return dps_; }

protected:

    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    DataPointSet&	dps_;

    CtxtIOObj&		ctio_;
    EM::ObjectID	horid_;
    BufferString	attrnm_;

    bool		acceptOK(CallBacker*);
    BufferString	getHorNm(EM::ObjectID);

    bool		doWork();

};


#endif
