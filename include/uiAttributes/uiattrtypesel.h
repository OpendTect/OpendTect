#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uigroup.h"

class uiComboBox;

/*!\brief Selector for attribute type. */

mExpClass(uiAttributes) uiAttrTypeSel : public uiGroup
{ mODTextTranslationClass(uiAttrTypeSel)
public:
				uiAttrTypeSel(uiParent*,bool sorted=true);
				~uiAttrTypeSel();
    void			fill(uiStringSet* selgroups=0);
						//!< with factory entries

    uiString			groupName() const;
    uiString			attributeDisplayName() const;
    const char*			attributeName() const;
    void			setGroupName(const uiString&);
    void			setAttributeName(const char*);
    void			setAttributeDisplayName(const uiString&);

    Notifier<uiAttrTypeSel>	selChg;

    void			add(const uiString& group,const uiString& attr);
    void			update();	//!< after a number of add()'s
    void			setEmpty();

    static uiString		sAllGroup();

protected:

    uiComboBox*			groupfld_;
    uiComboBox*			attrfld_;

    uiStringSet			groupnms_;
    uiStringSet			attrnms_;
    TypeSet<int>		groupidxs_;
    bool			sorted_;
    int*			sortidxs_;

    void			groupSel(CallBacker*);
    void			attrSel(CallBacker*);
    int				curGroupIdx() const;
    void			updAttrNms(const uiString* s=0);
    bool			isGroupDef(const uiString&) const;

    void			clear();
};
