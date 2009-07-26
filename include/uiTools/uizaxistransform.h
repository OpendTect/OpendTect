#ifndef uizaxistransform_h
#define uizaxistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id: uizaxistransform.h,v 1.1 2009-07-26 03:59:05 cvskris Exp $
________________________________________________________________________

-*/

#include "factory.h"
#include "fixedstring.h"
#include "uidlggroup.h"

class ZAxisTransform;
class uiGenInput;
class uiPushButton;
class uiDialog;

/*! Base class for ZAxisTransform ui's*/

mClass uiZAxisTransform : public uiDlgGroup
{
public:
    mDefineFactory2ParamInClass(uiZAxisTransform,uiParent*,const char*,factory);

    virtual ZAxisTransform*	getSelection()			= 0;
    virtual const char*		selName() const			= 0;
    virtual FixedString		getZDomain() const 		= 0;

protected:
    				uiZAxisTransform(uiParent*);
};


/*!Selects a ZAxisTransform. */
mClass uiZAxisTransformSel : public uiGroup
{
public:
    				uiZAxisTransformSel(uiParent*, bool withnone,
						    const char* fromdomain=0);
    ZAxisTransform*		getSelection();
    NotifierAccess*		selectionDone();
    int				nrTransforms() const;
    FixedString			getZDomain() const;
    bool			acceptOK();

protected:
    void			selCB(CallBacker*);
    void			settingsCB(CallBacker*);

    BufferString		fromdomain_;
    uiGenInput*			selfld_;
    uiPushButton*		settingsbut_;
    ObjectSet<uiZAxisTransform>	transflds_;
    ObjectSet<uiDialog>		transdlgs_;
};


#endif
