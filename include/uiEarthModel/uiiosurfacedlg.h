#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiiosurface.h"

class CtxtIOObj;
class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiSurfaceRead;
class uiSurfaceWrite;


namespace EM
{
    class Horizon3D;
    class Surface;
    class SurfaceIODataSelection;
}


/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiWriteSurfaceDlg : public uiDialog
{ mODTextTranslationClass(uiWriteSurfaceDlg);
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Surface&,
				          float shift);

    void		getSelection(EM::SurfaceIODataSelection&);
    bool		replaceInTree() const;
    const IOObj*	ioObj() const;

protected:
    uiSurfaceWrite*	iogrp_;
    const EM::Surface&	surface_;

    bool		acceptOK();
};


mExpClass(uiEarthModel) uiReadSurfaceDlg : public uiDialog
{ mODTextTranslationClass(uiReadSurfaceDlg);
public:
			uiReadSurfaceDlg(uiParent*,const char* type);

    const IOObj*	ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:
    uiSurfaceRead*	iogrp_;
    bool		acceptOK();
};


mExpClass(uiEarthModel) uiStoreAuxData : public uiDialog
{ mODTextTranslationClass(uiStoreAuxData);
public:
			uiStoreAuxData(uiParent*,const EM::Horizon3D&);

    bool		doOverWrite() const	{ return dooverwrite_; }
    const char*		auxdataName() const;

protected:
    uiGenInput*		attrnmfld_;
    const EM::Horizon3D& surface_;

    bool		dooverwrite_;
    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK();
};


mExpClass(uiEarthModel) uiCopySurface : public uiDialog
{ mODTextTranslationClass(uiCopySurface);
public:
			uiCopySurface(uiParent*,const IOObj&,
				      const uiSurfaceRead::Setup&);
			~uiCopySurface();

protected:

    uiSurfaceRead*	inpfld;
    uiIOObjSel*		outfld;

    CtxtIOObj&		ctio_;

    CtxtIOObj*		mkCtxtIOObj(const IOObj&);
    bool		acceptOK();

};


