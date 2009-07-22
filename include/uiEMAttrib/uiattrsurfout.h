#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.h,v 1.8 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattremout.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiGenInput;
class uiIOObjSel;

namespace Attrib { class DescSet; }

/*! \brief
Surface Output Batch dialog.
Used for calculating attributes on surfaces
*/


mClass uiAttrSurfaceOut : public uiAttrEMOut
{
public:
    			uiAttrSurfaceOut(uiParent*,const Attrib::DescSet&,
					 const NLAModel*,const MultiID&);
			~uiAttrSurfaceOut();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    void		attribSel(CallBacker*);
    void		objSel(CallBacker*);

    CtxtIOObj&		ctio_;
    
    uiGenInput*		attrnmfld_;
    uiIOObjSel*		objfld_;
};

#endif
