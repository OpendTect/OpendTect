#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.h,v 1.1 2004-10-04 16:09:59 nanne Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

class AttribDescSet;
class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiAttrSel;
class uiIOObjSel;


/*! \brief
Surface Output Batch dialog.
Used for calculating attributes on surfaces
*/


class uiAttrSurfaceOut : public uiFullBatchDialog
{
public:
    			uiAttrSurfaceOut(uiParent*,const AttribDescSet&,
					 const NLAModel*,const MultiID&);
			~uiAttrSurfaceOut();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    void		addNLA(int&);

    CtxtIOObj&		ctio;
    AttribDescSet&	ads;
    const MultiID&	nlaid;
    const NLAModel*	nlamodel;

    uiAttrSel*		attrfld;
    uiIOObjSel*		objfld;
};



#endif
