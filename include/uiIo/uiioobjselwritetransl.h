#ifndef uiioobjselwritetransl_h
#define uiioobjselwritetransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"

class IOObj;
class CtxtIOObj;
class Translator;
class IOObjContext;
class uiLabel;
class uiButton;
class uiComboBox;


/*!\brief Group for selecting output translator */

mExpClass(uiIo) uiIOObjSelWriteTranslator : public uiGroup
{
public:
			uiIOObjSelWriteTranslator(uiParent*,const CtxtIOObj&,
					    bool withopts=false);
			~uiIOObjSelWriteTranslator();

    bool		isEmpty() const		{ return !selfld_; }
    const Translator*	selectedTranslator() const;

    IOObj*		mkEntry(const char*) const;
    void		use(const IOObj&);

    uiObject*		endObj(bool left);

protected:

    IOObjContext&	ctxt_;
    ObjectSet<const Translator> trs_;

    uiComboBox*		selfld_;
    uiButton*		optsbut_;
    uiLabel*		lbl_;

    int			translIdx() const;

};


#endif
