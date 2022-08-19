#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "position.h"

class uiGenInput;
class uiTable;


mExpClass(uiTools) uiPositionTable : public uiGroup
{ mODTextTranslationClass(uiPositionTable);
public:
			uiPositionTable(uiParent*,bool withxy,bool withic,
					bool withz);

    void		setCoords(const TypeSet<Coord>&);
    void		getCoords(TypeSet<Coord>&) const;
    void		setBinIDs(const TypeSet<BinID>&);
    void		getBinIDs(TypeSet<BinID>&) const;

    void		setZRange(const Interval<float>&);
    void		getZRange(Interval<float>&) const;

    void		setRowColor(int rid,bool outsiderg);

protected:

    void		posChgCB(CallBacker*);
    int			getXCol() const;
    int			getYCol() const;
    int			getICol() const;
    int			getCCol() const;

    uiTable*		table_;
    uiGenInput*		zfld_;

    bool		withxy_;
    bool		withic_;
    bool		withz_;
};
