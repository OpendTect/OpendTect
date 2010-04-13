#ifndef uigmtfaults_h
#define uigmtfaults_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.h,v 1.3 2010-04-13 08:31:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;
class uiSelLineStyle;
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
    void			faultSel(CallBacker*);


    uiIOObjSel*			faultfld_;
    uiGenInput*			namefld_;
    uiGenInput*			optionfld_;
    uiGenInput*			zvaluefld_;
    uiIOObjSel*			horfld_;
    uiSelLineStyle*		linestfld_;
};

#endif
