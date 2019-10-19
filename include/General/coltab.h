#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
________________________________________________________________________

-*/


#include "generalmod.h"
#include "color.h"
#include "typeset.h"
#include "samplingdata.h"


namespace ColTab
{

typedef float				ValueType;
typedef float				PosType;
typedef SamplingData<ValueType>		SamplingType;
typedef float_twins			ClipRatePair;
enum SeqUseMode
{
    UnflippedSingle, UnflippedCyclic, FlippedSingle, FlippedCyclic
};
inline PosType getLimitedRelPos( PosType relpos )
{
    return getLimited( relpos, 0.f, 1.f );
}

mGlobal(General) bool			isFlipped(SeqUseMode);
mGlobal(General) bool			isCyclic(SeqUseMode);
mGlobal(General) SeqUseMode		getSeqUseMode(bool flipped,bool cyclic);
mGlobal(General) BufferString		toString(SeqUseMode);
mGlobal(General) void			toPar(SeqUseMode,IOPar&);
mGlobal(General) bool			fromPar(const IOPar&,SeqUseMode&);
mGlobal(General) void			convToPerc(ClipRatePair&);
mGlobal(General) void			convFromPerc(ClipRatePair&);
mGlobal(General) inline const char*	sKeySeqUseMode()
					{ return "Color Table Use Mode"; }

mGlobal(General) const char*		defSeqName(bool for_seismics);
mGlobal(General) ClipRatePair		defClipRate();
mGlobal(General) bool			defHistEq();
mGlobal(General) void			setDefSeqName(bool forseis,const char*);
mGlobal(General) void			setMapperDefaults(ClipRatePair,
							  bool histeq);


class Mapper;
class Sequence;
class TableFiller;
class IndexTableFiller;

/*!\brief Table of colors at regular value positions.

  You usually create the table from a Sequence and a Mapper. The
  Mapper::seqPosition() and Sequence::color() calls can be quite costly.
  For standard jobs involving a lot of data you cannot afford that. So, you
  need to pre-calculate a fixed number of exact colors and snap to those, or,
  if you can want to and you can afford it, interpolate. So, you use this class.

  The indexFor() and getColorAt() functions do *not* check on undef or rubbish
  being passed. Check this yourself and take appropriate action. You *can* pass
  values outside the range though (returns first or last index or color).

*/

mExpClass(General) Table
{
public:

    typedef TypeSet<Color>	SetType;

			Table(const Sequence&,int sz,SeqUseMode);
			Table(const Sequence&,int sz,const Mapper&);
			Table(const SetType&,SamplingType);

    int			size() const		{ return cols_.size(); }
    Color		color( int idx ) const	{ return cols_[idx]; }
    SamplingType	sampling() const	{ return sampling_; }

    int			indexFor(ValueType) const; //!< no udf protection
    Color		colorAt(ValueType) const; //!< no udf protection

    inline Color	snappedColorAt( ValueType val ) const
			{ return cols_[ indexFor(val) ]; }

protected:

    SetType		cols_;
    SamplingType	sampling_;

    friend class	TableFiller;
    void		setColor(const Sequence&,const Mapper&,int);

private:

    void		createTable(const Sequence& seq,const Mapper&,int);

};


/*!\brief Table of color indexes at regular value positions. the indexes point
  direcly into the colors from a ColTab::Sequence.

  You usually create the index table from a Mapper. The Mapper::seqPosition()
  call can be quite costly. So here you can pre-create a table of indexes.

  The object does *not* check on undef or rubbish values. Check this yourself
  and take appropriate action. You *can* pass values outside the range though
  (returns first or last index).

*/

mExpClass(General) IndexTable
{
public:

    typedef TypeSet<int> SetType;

			IndexTable(int sz,SeqUseMode);
			IndexTable(int sz,const Mapper&);
			IndexTable(const SetType&,SamplingType);

    int			size() const			{ return idxs_.size(); }
    int			colIndex( int idx ) const	{ return idxs_[idx]; }
    SamplingType	sampling() const		{ return sampling_; }

    int			indexFor(ValueType) const; //!< no udf protection

protected:

    SetType		idxs_;
    SamplingType	sampling_;

    void		setIndex(const Mapper&,int);
    friend class	IndexTableFiller;

private:

    void		createTable(const Mapper&,int);

};


} // namespace ColTab
