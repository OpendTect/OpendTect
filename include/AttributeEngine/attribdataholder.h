#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.11 2005-09-15 09:06:31 cvshelene Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "samplingdata.h"
#include "sets.h"

namespace Attrib
{

    /*!\brief Holds the data used in the attribute data

      Basically, this is a set of ValueSeries<float> objects, the size of
      each of these, and the start Z in the AE Z-Axis definition:
      N = N times the Z step. z0_ is therefore the amount of steps away from 0.

      The AE will work with any type of ValueSeries<float>. Internally,
      ArrayValueSeries<float> objects are always allocated.

      */

class DataHolder
{
public:
			    DataHolder( int z0, int nrsamples )
			    : z0_(z0), nrsamples_(nrsamples)
			    { data_.allowNull(true); }

			    ~DataHolder()		{ deepErase(data_); }

    inline DataHolder*	        clone() const;
    inline ValueSeries<float>*	add(bool addnull=false);
    				//!< Adds an ArrayValueSeries if !addnull

    int				nrSeries() const	{ return data_.size(); }
    ValueSeries<float>*		series( int idx ) const	{ return data_[idx]; }
    void			replace(int idx,ValueSeries<float>* valseries)
				{ ValueSeries<float>* ptr = 
				    data_.replace( idx, valseries ); 
				  if ( ptr ) delete ptr;
				}
    inline bool                 dataPresent(int samplenr) const;
    inline TypeSet<int>		validSeriesIdx() const;

    int				z0_;	//!< See class comments
    int				nrsamples_;

protected:

    ObjectSet< ValueSeries<float> >	data_;

};


inline ValueSeries<float>* DataHolder::add( bool addnull )
{
    ValueSeries<float>* res = addnull ? 0
	: new ArrayValueSeries<float>( new float[nrsamples_], true );
    data_ += res;
    return res;
}


inline bool DataHolder::dataPresent( int samplenr ) const
{
    if ( samplenr >= z0_ && samplenr < (z0_ + nrsamples_) )
	return true;

    return false;
}


inline DataHolder* DataHolder::clone() const
{
    DataHolder* dh = new DataHolder( z0_, nrsamples_ );
    dh->data_.allowNull(true);

    for ( int idx=0; idx<nrSeries(); idx++ )
    {
      if ( !data_[idx] ) dh->add( true );
      else
      {
	  dh->add();
	  memcpy( dh->data_[idx]->arr(), data_[idx]->arr(),
		  nrsamples_*sizeof(float) );
      }
    }

    return dh;
}


inline TypeSet<int> DataHolder::validSeriesIdx() const
{
    TypeSet<int> seriesidset;
    for( int idx=0; idx<nrSeries(); idx++ )
    {
	if ( data_[idx] )
	    seriesidset += idx;
    }

    return seriesidset;
}
}; //Namespace


#endif
