#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.5 2005-05-12 10:53:22 cvshelene Exp $
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
    void			replace(ValueSeries<float>* valseries,int idx)
				{ data_.replace( valseries, idx ); }



    int				t0_;
    int				nrsamples_;

protected:

    ObjectSet< ValueSeries<float> >	data_;

};


ValueSeries<float>* DataHolder::add(bool null)
{
    ValueSeries<float>* res = null
	? 0 : new ArrayValueSeries<float>(new float[nrsamples_]);
    data_ += res;
    return res;
}


}; //Namespace


#endif
