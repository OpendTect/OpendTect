#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uigroup.h"

#include "ptrman.h"
#include "bufstringset.h"

class uiComboBox;

/*!\brief Selector for attribute type

  Note that every attribute belongs to a group, but the usage of that
  is not mandatory.

  */

mExpClass(uiAttributes) uiAttrTypeSel : public uiGroup
{
public:
				uiAttrTypeSel(uiParent*,bool sorted=true);
				~uiAttrTypeSel();

    void			fill(BufferStringSet* selgrps=0);
						//!< with factory entries

    const char*			group() const;
    const char*			attr() const;
    void			setGrp(const char*);
    void			setAttr(const char*);

    Notifier<uiAttrTypeSel>	selChg;

    void			add(const char* grp,const char* attr);
    void			update();	//!< after a number of add()'s
    void			setEmpty();

    static const char*		sKeyAllGrp;

protected:

    uiComboBox*			grpfld_;
    uiComboBox*			attrfld_;

    BufferStringSet		grpnms_;
    BufferStringSet		attrnms_;
    TypeSet<int>		attrgroups_;
    ConstArrPtrMan<int>		idxs_		= nullptr;
    bool			sorted_;

    void			grpSel(CallBacker*);
    void			attrSel(CallBacker*);
    int				curGrpIdx() const;
    void			updAttrNms(const char* s=0);
    bool			isPrefAttrib(int,const char*) const;

    void			clear();
};
