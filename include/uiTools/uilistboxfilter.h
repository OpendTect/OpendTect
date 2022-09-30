#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigeninput.h"
class uiListBox;


/*!\brief A filter field attaching itself to a listbox. */

mExpClass(uiTools) uiListBoxFilter : public uiGenInput
{ mODTextTranslationClass(uiListBoxFilter)
public:
			uiListBoxFilter(uiListBox&,bool above=true);
			~uiListBoxFilter();

    void		setItems(const BufferStringSet&);
    void		setItems(const uiStringSet&);
    void		setFilter(const char*);
    void		setEmpty();

    int			getCurrent() const;
    void		getChosen(TypeSet<int>&) const;
    void		getChosen(BufferStringSet&) const;
    int			nrChosen() const;

    void		removeItem(int);

    Notifier<uiListBoxFilter> newFilter;

protected:

    uiListBox&		lb_;
    BufferStringSet	availitems_;

    void		filtChg(CallBacker*);

};
