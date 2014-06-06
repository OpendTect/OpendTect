#ifndef uilistboxfilter_h
#define uilistboxfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jun 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigeninput.h"
class uiListBox;


/*!\brief A filter field attaching itself to a listbox. */

mExpClass(uiTools) uiListBoxFilter : public uiGenInput
{
public:

			uiListBoxFilter(uiListBox&,bool above=true);

    void		setItems(const BufferStringSet&);
    void		setItems(const TypeSet<uiString>&);
    void		setFilter(const char*);

    int			getCurrent() const;
    void		getChosen(TypeSet<int>&) const;

    Notifier<uiListBoxFilter> newFilter;

protected:

    uiListBox&		lb_;
    BufferStringSet	availitems_;

    void		filtChg(CallBacker*);

};



#endif
