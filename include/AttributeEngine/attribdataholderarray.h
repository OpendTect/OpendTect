#ifndef attribdataholderarray_h
#define attribdataholderarray_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: attribdataholderarray.h,v 1.2 2007-02-19 16:41:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "arraynd.h"


namespace Attrib
{

class DataHolder;

class DataHolderArray : public Array3D<float>
{
public:
			DataHolderArray(const ObjectSet<DataHolder>&);
			~DataHolderArray();

    void		set(int,int,int,float);
    float		get(int,int,int) const;
    const Array3DInfo&	info() const		{ return info_; }

protected:

    Array3DInfoImpl	info_;
    ObjectSet<DataHolder> dh_;
};

}

#endif
