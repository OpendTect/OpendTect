#ifndef attribdataholderarray_h
#define attribdataholderarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "arraynd.h"


namespace Attrib
{

class DataHolder;

/*!
\brief An array of attribute data holders.
*/
mExpClass(AttributeEngine) DataHolderArray : public Array3D<float>
{
public:
			DataHolderArray(const ObjectSet<DataHolder>&);//type_ 0
			DataHolderArray(const ObjectSet<DataHolder>& dh, 
				int sidx,int dim0sz,int dim1sz);

			~DataHolderArray();

    void		set(int,int,int,float);
    float		get(int,int,int) const;
    const Array3DInfo&	info() const		{ return info_; }

    virtual void	getAll(float*) const;
    virtual void	getAll(ValueSeries<float>& vs) const;

protected:

    Array3DInfoImpl	info_;
    ObjectSet<DataHolder> dh_;
    char		type_;
    int			seriesidx_;	
};

}

#endif

