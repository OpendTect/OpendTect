#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uigroup.h"
class uiComboBox;
namespace Strat { class Content; class RefTree; }


/*!\brief Gets the layer content */

mExpClass(uiStrat) uiStratLayerContent : public uiGroup
{ mODTextTranslationClass(uiStratLayerContent)
public:

    typedef Strat::Content	Content;
    typedef Strat::RefTree	RefTree;

			uiStratLayerContent(uiParent*,bool isfinal,
					    const RefTree&);

    void		set(const Content&);
    const Content& get() const;

    int			selectedIndex() const;
    void		setSelectedIndex(int);
    int			addOption(const char*);

    Notifier<uiStratLayerContent> contentSelected;

protected:

    uiComboBox*		fld_;
    const RefTree&	rt_;

    void		contSel(CallBacker*);

};
