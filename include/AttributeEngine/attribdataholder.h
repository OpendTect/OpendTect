#ifndef attribdataholder_h
#define attribdataholder_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdataholder.h,v 1.3 2005-05-09 14:40:01 cvshelene Exp $
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
				DataHolder( int t1, int nrsamples )
				: t1_(t1), nrsamples_(nrsamples)
				{ data_.allowNull(true); }

				~DataHolder()		{ deepErase(data_); }

    inline ValueSeries<float>*	add();

    int				nrItems() const	{ return data_.size(); }
    ValueSeries<float>*		item( int idx )	const { return data_[idx]; }
    void			replace(ValueSeries<float>* valseries,int idx)
				{ data_.replace( valseries, idx ); }



    int				t1_;
    int				nrsamples_;

protected:

    ObjectSet< ValueSeries<float> >	data_;

};


ValueSeries<float>* DataHolder::add()
{
    float* ptr = new float[nrsamples_];
    ValueSeries<float>* res = new ArrayValueSeries<float>(ptr);
    data_ += res;
    return res;
}


}; //Namespace


#endif
