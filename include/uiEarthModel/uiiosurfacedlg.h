#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.24 2012-08-03 13:00:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiiosurface.h"

class CtxtIOObj;
class IOObj;
class MultiID;
class uiGenInput;
class uiIOObjSel;
class uiSurfaceRead;
class uiSurfaceWrite;

namespace EM { class Surface; class SurfaceIODataSelection; class Horizon3D; }


/*! \brief Dialog for horizon export */

mClass(uiEarthModel) uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Surface&,
				          float shift);

    void		getSelection(EM::SurfaceIODataSelection&);
    bool		replaceInTree() const;
    IOObj*		ioObj() const;

protected:
    uiSurfaceWrite*	iogrp_;
    const EM::Surface&	surface_;

    bool		acceptOK(CallBacker*);
};


mClass(uiEarthModel) uiReadSurfaceDlg : public uiDialog
{
public:
			uiReadSurfaceDlg(uiParent*,const char* type);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:
    uiSurfaceRead*	iogrp_;
    bool		acceptOK(CallBacker*);
};


mClass(uiEarthModel) uiStoreAuxData : public uiDialog
{
public:
    			uiStoreAuxData(uiParent*,const EM::Horizon3D&);

    bool		doOverWrite() const	{ return dooverwrite_; }
    const char*		auxdataName() const;

protected:
    uiGenInput*		attrnmfld_;
    const EM::Horizon3D& surface_;

    bool		dooverwrite_;
    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};


mClass(uiEarthModel) uiCopySurface : public uiDialog
{
public:
    			uiCopySurface(uiParent*,const IOObj&,
				      const uiSurfaceRead::Setup&);
			~uiCopySurface();

    void		setInputKey(const char* key);
protected:
    uiSurfaceRead*	inpfld;
    uiIOObjSel*		outfld;

    CtxtIOObj&		ctio_;
    
    CtxtIOObj*		mkCtxtIOObj(const IOObj&);
    bool		acceptOK(CallBacker*);
};

#endif

