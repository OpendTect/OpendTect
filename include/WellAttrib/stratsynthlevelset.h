#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		January 2017
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "color.h"
#include "typeset.h"
#include "valseriesevent.h"
#include "stratsynthlevel.h"


mExpClass(WellAttrib) StratSynthLevelSet : public StratSynthLevel
{
public:
    typedef TypeSet<float> LVLZVals;
    typedef TypeSet< LVLZVals > LVLZValsSet;

    StratSynthLevelSet(const BufferStringSet&,const LVLZValsSet&);
    StratSynthLevelSet(const StratSynthLevel*);

    StratSynthLevelSet&		operator =(StratSynthLevelSet&);
    BufferStringSet&		getLevelNmSet() { return lvlnmset_; }
    LVLZValsSet&		getLevelZVals() { return zvals_; }
    const int			size() { return  lvlnmset_.size(); }
    void			addData(BufferString&,LVLZVals&);
    void			addDatas(BufferStringSet&,LVLZValsSet&);
    VSEvent::Type&		getSnapEv() { return snapev_; }
    void			setSnapEv(const VSEvent::Type& snapev)
				{ snapev_ = snapev; }
    void			setEmpty();
    StratSynthLevel*		getStratLevel(const int idx=0);
protected:
    BufferStringSet		lvlnmset_;
    LVLZValsSet			zvals_;
    VSEvent::Type		snapev_;
    TypeSet<StratSynthLevel*>	stratsynthlvl_;

};
