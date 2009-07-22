#ifndef uigmtsymbolpars_h
#define uigmtsymbolpars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
 RCS:		$Id: uigmtsymbolpars.h,v 1.3 2009-07-22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class IOPar;
class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;

class uiGMTSymbolPars : public uiGroup
{
public:

    			uiGMTSymbolPars(uiParent*);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    uiComboBox*		shapefld_;
    uiGenInput*		sizefld_;
    uiCheckBox*		fillfld_;
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;

    void		fillSel(CallBacker*);
    void		fillShapes();
};


#endif
