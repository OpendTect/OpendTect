#ifndef uigmtfaults_h
#define uigmtfaults_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.h,v 1.2 2010-04-07 09:24:09 cvsnageswara Exp $
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
    void			loadFault(CallBacker*);
    bool			loadSurface(CallBacker*);


    uiIOObjSel*			faultfld_;
    uiGenInput*			namefld_;
    uiGenInput*			optionfld_;
    uiGenInput*			zvaluefld_;
    uiIOObjSel*			horfld_;
    uiSelLineStyle*		linestfld_;
};

#endif
