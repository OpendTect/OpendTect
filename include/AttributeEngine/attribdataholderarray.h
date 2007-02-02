#ifndef attribdataholderarray_h
#define attribdataholderarray_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: attribdataholderarray.h,v 1.1 2007-02-02 15:42:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "arraynd.h"


namespace Attrib
{

class DataHolder;
class Data2DHolder;

// TODO: Finish
class DataHolderArray : public Array3D<float>
{
public:
    			DataHolderArray(const ObjectSet<DataHolder>&)	{}
			~DataHolderArray()				{}
protected:

};


class Data2DHolderArray : public Array3D<float>
{
public:
			Data2DHolderArray(Data2DHolder&);
			~Data2DHolderArray();

    void		set(int,int,int,float);
    float		get(int,int,int) const;
    const Array3DInfo&	info() const		{ return info_; }

protected:

    Array3DInfoImpl	info_;
    Data2DHolder&	dh_;
    TypeSet<int>	trcidxs_;
};

}

#endif
