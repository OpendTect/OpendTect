#ifndef uiattrtrcselout_h
#define uiattrtrcselout_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.h,v 1.3 2005-12-28 18:14:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "attribdescid.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiBinIDSubSel;
class HorSampling;

namespace Attrib { class DescSet; }

/*! \brief
Trace Selection Output Batch dialog.
Used for calculating attributes between surfaces or withing a user-defined 
interval around a surface
*/


class uiAttrTrcSelOut : public uiFullBatchDialog
{
public:
    			uiAttrTrcSelOut(uiParent*,const Attrib::DescSet&,
				      const NLAModel*,const MultiID&,bool);
			~uiAttrTrcSelOut();
    void		getComputableSurf(HorSampling&);

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    bool		addNLA(Attrib::DescID&);
    void                objSel(CallBacker*);
    void                attrSel(CallBacker*);
    void		extraZSel(CallBacker*);

    CtxtIOObj&		ctio;
    CtxtIOObj&		ctio2;
    CtxtIOObj&		ctioout;
    Attrib::DescSet&	ads;
    const MultiID&	nlaid;
    const NLAModel*	nlamodel;

    uiAttrSel*		attrfld;
    uiIOObjSel*		objfld;
    uiIOObjSel*		obj2fld;
    uiGenInput*		extrazfld;
    uiGenInput*		gatefld;
    uiBinIDSubSel*	subselfld;
    uiGenInput*		outsidevalfld;
    uiSeisSel*          outpfld;
    bool		usesinglehor_;
};

#endif
