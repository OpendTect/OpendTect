#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          February 2003
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

    void		setBinIDs(const TypeSet<BinID>&);
    void		getBinIDs(TypeSet<BinID>&) const;

    void		setZRange(const Interval<float>&);
    void		getZRange(Interval<float>&) const;

protected:

    uiTable*		table_;
    uiGenInput*		zfld_;

    bool		withz_;
};
