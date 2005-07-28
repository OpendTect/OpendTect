#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.h,v 1.3 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;

namespace Attrib { class DescSet; }

/*! \brief
Surface Output Batch dialog.
Used for calculating attributes on surfaces
*/


class uiAttrSurfaceOut : public uiFullBatchDialog
{
public:
    			uiAttrSurfaceOut(uiParent*,const Attrib::DescSet&,
					 const NLAModel*,const MultiID&);
			~uiAttrSurfaceOut();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    bool		addNLA(int&);
    void		attribSel(CallBacker*);

    CtxtIOObj&		ctio;
    Attrib::DescSet&	ads;
    const MultiID&	nlaid;
    const NLAModel*	nlamodel;

    uiAttrSel*		attrfld;
    uiGenInput*		attrnmfld;
    uiIOObjSel*		objfld;
};

#endif
