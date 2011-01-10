#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.h,v 1.9 2011-01-10 10:20:57 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattremout.h"

class CtxtIOObj;
class IOPar;
class MultiID;
class NLAModel;
class uiArray2DInterpolSel;
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

   void			fillGridPar(IOPar&) const;
protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    void		attribSel(CallBacker*);
    void		objSel(CallBacker*);

    CtxtIOObj&		ctio_;
    
    uiGenInput*		attrnmfld_;
    uiIOObjSel*		objfld_;
    uiArray2DInterpolSel* interpolfld_;
};

#endif
