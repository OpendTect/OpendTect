#ifndef uistratlaycontent_h
#define uistratlaycontent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
 RCS:           $Id: uistratlaycontent.h,v 1.2 2012/08/10 08:38:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiComboBox;
namespace Strat { class Content; class RefTree; }


/*!\brief Gets the layer content */

mClass uiStratLayerContent : public uiGroup
{
public:

  			uiStratLayerContent(uiParent*,bool isfinal,
					    const Strat::RefTree&);

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


#endif
