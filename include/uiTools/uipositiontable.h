#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
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
