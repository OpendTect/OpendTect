#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiiosurface.h"

class CtxtIOObj;
class Executor;
class IOObj;
class MultiID;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
class uiSurfaceRead;
class uiSurfaceWrite;

namespace EM 
{ 
    class Fault3D;
    class Horizon3D; 
    class Surface; 
    class SurfaceIODataSelection; 
}


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

/*Brief dialog for saving fault aux-data. */
mClass(uiEarthModel) uiStoreFaultData : public uiDialog
{
public:
    			uiStoreFaultData(uiParent*,const EM::Fault3D&);
    			~uiStoreFaultData();

    Executor*		dataSaver(); 			
    bool		doOverWrite() const	{ return dooverwrite_; }
    const char*		surfaceDataName() const;
    int			surfaceDataIdx() const;

protected:

    void		selButPushedCB(CallBacker*);

    uiPushButton*	selbut_;
    uiGenInput*		attrnmfld_;
    const EM::Fault3D&	surface_;

    bool		dooverwrite_;
    bool		acceptOK(CallBacker*);
};


#endif

