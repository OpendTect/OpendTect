#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistring.h"

class uiGenInput;
class uiTable;

mExpClass(uiTools) uiBinIDTable : public uiGroup
{ mODTextTranslationClass(uiBinIDTable);
public:
			uiBinIDTable(uiParent*,bool withz);
			~uiBinIDTable();

    void		setBinIDs(const TypeSet<BinID>&);
    void		getBinIDs(TypeSet<BinID>&) const;

    void		setZRange(const Interval<float>&);
    void		getZRange(Interval<float>&) const;

protected:

    uiTable*		table_;
    uiGenInput*		zfld_;

    bool		withz_;
};
