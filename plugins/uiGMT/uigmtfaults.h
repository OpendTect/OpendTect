#ifndef uigmtfaults_h
#define uigmtfaults_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.h,v 1.4 2011-04-27 08:58:46 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
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


    uiGenInput*			namefld_;
    uiGenInput*			optionfld_;
    uiGenInput*			zvaluefld_;
    uiIOObjSel*			horfld_;
    uiIOObjSelGrp*		faultfld_;
    uiSelLineStyle*		linestfld_;
};

#endif
