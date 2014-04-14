#ifndef uit2dconvsel_h
#define uit2dconvsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"
#include "uizaxistransform.h"

class uiIOObjSel;
class uiGenInput;
class uiComboBox;
class uiT2DConvSelGroup;


/*! \brief single-line object for selecting T to depth conversion. */

mExpClass(uiIo) uiT2DConvSel : public uiZAxisTransformSel
{ mODTextTranslationClass(uiT2DConvSel);
public:

    mExpClass(uiIo) Setup
    {
    public:
			Setup(uiIOObjSel* tiedto,bool opt=true);

	mDefSetupMemb(BufferString,fldtext)
	mDefSetupMemb(bool,optional)
	mDefSetupMemb(uiIOObjSel*,tiedto)
	mDefSetupMemb(bool,ist2d)
    };

			uiT2DConvSel(uiParent*,const Setup&);

protected:

    Setup				setup_;

    void				inpSel(CallBacker*);
};

#endif

