#ifndef uigmtfaults_h
#define uigmtfaults_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.h,v 1.1 2010-03-26 11:59:57 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;
class uiSelLineStyle;
class CtxtIOObj;
class IOPar;

mClass uiGMTFaultsGrp : public uiGMTOverlayGrp
{
public:
    static void         initClass();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:
				uiGMTFaultsGrp(uiParent*);

    static uiGMTOverlayGrp*     createInstance(uiParent*);
    static int                  factoryid_;

    void			typeChgCB(CallBacker*);
    bool			loadFault(CallBacker*);
    bool			loadSurface(CallBacker*);


    CtxtIOObj&			faultctio_;
    CtxtIOObj&			surfacectio_;
    uiIOObjSel*			faultfld_;
    uiGenInput*			namefld_;
    uiGenInput*			optionfld_;
    uiGenInput*			zvaluefld_;
    uiIOObjSel*			surfacefld_;
    uiSelLineStyle*		linestfld_;
};

#endif
