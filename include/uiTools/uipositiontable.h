#ifndef uipositiontable_h
#define uipositiontable_h

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uigroup.h"
#include "position.h"

class uiGenInput;
class uiTable;
template <class T> class Interval;

mClass uiPositionTable : public uiGroup
{
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

#endif
