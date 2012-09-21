#ifndef uiattrsurfout_h
#define uiattrsurfout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattremout.h"

class IOPar;
class MultiID;
class NLAModel;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
class Array2DInterpol;
class BufferString;

namespace Attrib { class DescSet; }

/*! \brief
Surface Output Batch dialog.
Used for calculating attributes on surfaces
*/


mClass(uiEMAttrib) uiAttrSurfaceOut : public uiAttrEMOut
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
    void		objSelCB(CallBacker*);
    void		fillUdfSelCB(CallBacker*);
    void		settingsCB(CallBacker*);

    uiGenInput*		attrnmfld_;
    uiIOObjSel*		objfld_;
    uiGenInput*		filludffld_;
    uiPushButton*	settingsbut_;
    Array2DInterpol*	interpol_;
    BufferString	methodname_;
};

#endif

