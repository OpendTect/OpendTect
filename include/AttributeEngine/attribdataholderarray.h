#ifndef attribdataholderarray_h
#define attribdataholderarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id: attribdataholderarray.h,v 1.7 2012-08-03 13:00:07 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "arraynd.h"


namespace Attrib
{

class DataHolder;

mClass(AttributeEngine) DataHolderArray : public Array3D<float>
{
public:
			DataHolderArray(const ObjectSet<DataHolder>&,
					bool manageset);//type_ 0
			DataHolderArray(const ObjectSet<DataHolder>& dh, 
				int sidx,int dim0sz,int dim1sz,bool manageset);

			~DataHolderArray();

    void		set(int,int,int,float);
    float		get(int,int,int) const;
    const Array3DInfo&	info() const		{ return info_; }

protected:

    Array3DInfoImpl	info_;
    ObjectSet<DataHolder> dh_;
    bool		manageset_;
    char		type_;
    int			seriesidx_;	
};

}

#endif

