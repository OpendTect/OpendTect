#ifndef uibinidtable_h
#define uibinidtable_h

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class BinID;
class uiGenInput;
class uiTable;
template <class T> class Interval;

mClass(uiTools) uiBinIDTable : public uiGroup
{
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

#endif

