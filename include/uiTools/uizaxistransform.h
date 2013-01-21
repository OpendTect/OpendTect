#ifndef uizaxistransform_h
#define uizaxistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert/Nanne
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "factory.h"
#include "fixedstring.h"
#include "uidlggroup.h"

class ZAxisTransform;
class uiGenInput;
class uiPushButton;
class uiDialog;

/*! Base class for ZAxisTransform ui's*/

mExpClass(uiTools) uiZAxisTransform : public uiDlgGroup
{
public:
    mDefineFactory3ParamInClass(uiZAxisTransform,uiParent*,
	    			const char*,const char*,factory);

    virtual ZAxisTransform*	getSelection()			= 0;
    virtual const char*		selName() const			= 0;
    virtual FixedString		getZDomain() const 		= 0;

protected:
    				uiZAxisTransform(uiParent*);
};


/*!Selects a ZAxisTransform. */
mExpClass(uiTools) uiZAxisTransformSel : public uiGroup
{
public:
    				uiZAxisTransformSel(uiParent*, bool withnone,
						    const char* fromdomain=0,
						    const char* todomain=0);
    bool			fillPar(IOPar&);
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

