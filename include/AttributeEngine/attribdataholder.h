#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.8 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "valseries.h"
#include "samplingdata.h"
#include "sets.h"

namespace Attrib
{

class DataHolder
{
public:
				DataHolder( int t0, int nrsamples )
				: t0_(t0), nrsamples_(nrsamples)
				{ data_.allowNull(true); }

				~DataHolder()		{ deepErase(data_); }

    inline ValueSeries<float>*	add(bool null=false);

    int				nrItems() const	{ return data_.size(); }
    ValueSeries<float>*		item( int idx )	const { return data_[idx]; }
    void			replace(int idx,ValueSeries<float>* valseries)
				{ data_.replace( idx, valseries ); }

    inline bool			append(const DataHolder& dh);

    int				t0_;
    int				nrsamples_;

protected:

    ObjectSet< ValueSeries<float> >	data_;

};


ValueSeries<float>* DataHolder::add( bool null )
{
    ValueSeries<float>* res = null
	? 0 : new ArrayValueSeries<float>( new float[nrsamples_], true );
    data_ += res;
    return res;
}


bool DataHolder::append( const DataHolder& dh )
{
    if ( nrItems() != dh.nrItems() )
	return false;
    
    int prevnrsamp = nrsamples_;
    nrsamples_ += dh.nrsamples_;
    for ( int idx=0; idx< dh.nrItems(); idx++ )
    {
	for ( int idy=0; idy<dh.nrsamples_; idy++ )
	    item(idx)->setValue( prevnrsamp+idy, dh.item(idx)->value(idy) );
    }
    return true;
}

}; //Namespace


#endif
