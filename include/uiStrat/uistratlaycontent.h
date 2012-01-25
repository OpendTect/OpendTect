#ifndef uistratlaycontent_h
#define uistratlaycontent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
 RCS:           $Id: uistratlaycontent.h,v 1.1 2012-01-25 16:07:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiComboBox;
namespace Strat { class Content; class RefTree; }


/*!\brief Gets the layer content */

mClass uiStratLayerContent : public uiGroup
{
public:

  			uiStratLayerContent(uiParent*,
					    const Strat::RefTree* rt=0);

    void		set(const Strat::Content&);
    const Strat::Content& get() const;

protected:

    uiComboBox*		fld_;
    const Strat::RefTree& rt_;
};


#endif
