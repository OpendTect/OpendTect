/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Prajjaval Singh
 Date:          January 2017
________________________________________________________________________

-*/

#include "stratsynthlevelset.h"
#include "stratsynthlevel.h"
#include "stratlevel.h"

StratSynthLevelSet::StratSynthLevelSet(  const BufferStringSet& lvlnmset,
					 const LVLZValsSet& dptset )
    : StratSynthLevel("",Color::Black())
    , snapev_(VSEvent::None)
    , lvlnmset_(lvlnmset)
{
    if ( dptset.size() > 0 )
    {
	const int sz = lvlnmset.size();
	zvals_.setSize(sz);
	for ( int lvlidx=0; lvlidx<sz; lvlidx++ )
	{
	    for ( int zidx=0; zidx<dptset[lvlidx].size(); zidx++ )
	    {
		zvals_[lvlidx] += dptset[lvlidx][zidx];
	    }
	    if ( dptset[lvlidx].size() == 0 )
	    {
		zvals_[lvlidx] += mUdf(float);
		continue;
	    }

	    Color clr = Strat::LVLS().getByName( lvlnmset_[lvlidx]->buf() )
				     .color();
	    stratsynthlvl_ += new StratSynthLevel( *lvlnmset_[lvlidx], clr,
							    &zvals_[lvlidx] );
	}
    }
    else
    {
	zvals_.setSize(1);
	zvals_[0] += mUdf(float);
    }
}


void StratSynthLevelSet::setEmpty()
{
    if ( !&lvlnmset_ ) return;
    lvlnmset_.setEmpty();
    for ( int idx=0; idx<zvals_.size(); idx++ )
	zvals_[idx].setEmpty();
}


StratSynthLevel* StratSynthLevelSet::getStratLevel( const int idx )
{
    return stratsynthlvl_[idx];
}


StratSynthLevelSet& StratSynthLevelSet::operator=( StratSynthLevelSet& lvlset )
{
    setEmpty();
    lvlnmset_ = lvlset.lvlnmset_;
    snapev_ = lvlset.snapev_;
    for( int idx=0; idx<lvlset.zvals_.size(); idx++ )
	for( int validx=0; validx<
	    lvlset.zvals_[idx].size(); validx++ )
	    zvals_[idx] += lvlset.zvals_[idx][validx];
    return (*this);
}


void StratSynthLevelSet::addData( BufferString& lvlnm, LVLZVals& lvlzvals )
{
    lvlnmset_.add(lvlnm);
    zvals_ += lvlzvals;
}


void StratSynthLevelSet::addDatas( BufferStringSet& lvlnmset,
						    LVLZValsSet& lvlzvalsset )
{
    for( int idx=0; idx<lvlnmset.size(); idx++ )
    {
	lvlnmset_.add( lvlnmset[idx] );
	zvals_ += lvlzvalsset[idx];
    }
}

