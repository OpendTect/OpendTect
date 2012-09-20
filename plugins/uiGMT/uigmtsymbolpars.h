#ifndef uigmtsymbolpars_h
#define uigmtsymbolpars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uigroup.h"

class IOPar;
class uiComboBox;
class uiGenInput;
class uiColorInput;


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
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;
    bool		usewellsymbols_;

    void		fillSel(CallBacker*);
    void		fillShapes();
};


#endif
