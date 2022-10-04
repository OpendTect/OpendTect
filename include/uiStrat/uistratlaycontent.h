#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

			uiStratLayerContent(uiParent*,bool isfinal,
					    const Strat::RefTree&);
			~uiStratLayerContent();

    void		set(const Strat::Content&);
    const Strat::Content& get() const;

    int			selectedIndex() const;
    void		setSelectedIndex(int);
    int			addOption(const char*);

    Notifier<uiStratLayerContent> contentSelected;

protected:

    uiComboBox*		fld_;
    const Strat::RefTree& rt_;

    void		contSel(CallBacker*);

};
