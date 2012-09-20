#ifndef uiisopachmaker_h
#define uiisopachmaker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "uigroup.h"
#include "emposid.h"

class uiIOObjSel;
class uiGenInput;
class CtxtIOObj;
class DataPointSet;
namespace EM { class EMObject; }

class IOPar;

/*! \brief Create isopach as attribute of horizon */

class uiIsopachMakerGrp : public uiGroup
{
public:
			uiIsopachMakerGrp(uiParent*,EM::ObjectID);
			~uiIsopachMakerGrp();

    bool		chkInputFlds();
    bool		fillPar(IOPar&);
    BufferString	getHorNm(EM::ObjectID);
    const char*		attrName() const;

protected:
    uiIOObjSel*		basesel_;
    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    uiGenInput*		msecsfld_;
    CtxtIOObj&		basectio_;
    CtxtIOObj&		ctio_;
    EM::ObjectID	horid_;
    EM::EMObject*	baseemobj_;
    
    void		toHorSel(CallBacker*);
};


class uiIsopachMakerBatch : public uiFullBatchDialog
{
public:

			uiIsopachMakerBatch(uiParent*);
protected:
    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    uiIsopachMakerGrp*	grp_;
    bool		isoverwrite_;
};


class uiIsopachMakerDlg : public uiDialog
{
public:
			uiIsopachMakerDlg(uiParent*,EM::ObjectID);
			~uiIsopachMakerDlg();

    const char*		attrName() const{ return grp_->attrName(); }
    const DataPointSet&	getDPS()		{ return *dps_; }

protected:    
    bool		acceptOK(CallBacker*);
    bool		doWork();

    uiIsopachMakerGrp*	grp_;
    DataPointSet*	dps_;
};
#endif
