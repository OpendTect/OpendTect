#ifndef uibinidtable_h
#define uibinidtable_h

/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id: uibinidtable.h,v 1.5 2009-01-08 07:07:01 cvsranojay Exp $
 ________________________________________________________________________

-*/

#include "uigroup.h"

class BinID;
class uiGenInput;
class uiTable;
template <class T> class Interval;

mClass uiBinIDTable : public uiGroup
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
