#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginecommon.h"
#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "refcount.h"
#include "valseries.h"

class SeisTrcInfo;

namespace Attrib
{

/*!
\brief Holds the attribute data.

  The Z is an indexing scheme which positions absolute Z==0 at index 0. So to get the index you
  evaluate (int)(z/step). But, the actual postions can be (and will be!) not at exact indexes.
  To get the real Z you have to add extrazfromsamppos_.

  The attribute engine will go through all inputs and determine the step (the 'reference step').
  So to be able to figure out the actual Z from an index, you need this global ref step.
  See zAtIdx().

  So what we have here is basically a set of ValueSeries<float> objects, the size of
  each of these, and the start Z index in the AE Z-Axis definition:
  N = N times the Z step. z0_ is therefore the amount of steps away from 0.

  The AE will work with any type of ValueSeries<float>. Internally,
  ArrayValueSeries<float,float> objects are always allocated.

  The class variable extrazfromsamppos_ is to keep track of an eventual
  exact position which would not be exactly on a sample ( in the case of
  horizons, picksets... )

  Beware! Each of the ValueSeries can be null.

*/

mExpClass(AttributeEngine) DataHolder
{
public:

			DataHolder(int z0,int nrsamples,float extraz=0);
			~DataHolder();

    DataHolder*	        clone() const;
    ValueSeries<float>*	add(bool addnull=false);
			//!< Adds an ArrayValueSeries if !addnull

    int			nrSeries() const		{ return data_.size(); }
    inline ValueSeries<float>* series( int idx ) const	{ return gtSer(idx); }
    void		replace(int idx,ValueSeries<float>*);
    bool                dataPresent(int samplenr) const;
    TypeSet<int>	validSeriesIdx() const;
    float		getValue(int serieidx,float exactz,float refstep) const;
    float		zAtIdx( int idx, float refstep ) const
			{ return (z0_ + idx) * refstep + extrazfromsamppos_; }

    inline bool		isEmpty() const		{ return nrSeries() == 0; }
    static float	getExtraZFromSampPos(float,float);
    static float	getExtraZAndSampIdxFromExactZ(float,float,int&);

    int			z0_;	//!< See class comments
    int			nrsamples_;
    float		extrazfromsamppos_;	//!< See class comments
    TypeSet<int>	classstatus_;  //each series can have a different status
				       // -1 Unknown
				       //  0 Interpolate
				       //  1 Classification
protected:

    ObjectSet< ValueSeries<float> >	data_;
    ValueSeries<float>*			gtSer(int) const;

};


/*!
\brief Class that holds 2d seismic data or attribute data.
*/

mExpClass(AttributeEngine) Data2DHolder : public RefCount::Referenced
{
public:

    inline int			size() const	{ return dataset_.size(); }
    TrcKeyZSampling		getTrcKeyZSampling() const;
    int				getDataHolderIndex(int) const;
    ObjectSet<DataHolder>	dataset_;
				/*!<\note that z0 on the dataholder refers
					  to samples in trcinfoset_.sampling. */
    ObjectSet<SeisTrcInfo>	trcinfoset_;
				/*!<\note that the sampling is the same
					  for all traces. */

    inline bool			isEmpty() const	{ return size() == 0; }
protected:
				~Data2DHolder();
};

} // namespace Attrib
