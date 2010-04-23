#ifndef uigmtsymbolpars_h
#define uigmtsymbolpars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
 RCS:		$Id: uigmtsymbolpars.h,v 1.4 2010-04-23 11:32:25 cvsnageswara Exp $
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
			uiGMTSymbolPars(uiParent*,bool);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    uiComboBox*		shapefld_;
    uiGenInput*		sizefld_;
    uiCheckBox*		fillfld_;
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;
    bool		usewellsymbols_;

    void		fillSel(CallBacker*);
    void		fillShapes();
};


#endif
