#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			DataHolderArray(const ObjectSet<DataHolder>&);
			/*Use for single inlines, crosslines or 2D lines
			  with multiple valueseries. type_=0 */

			DataHolderArray(const ObjectSet<DataHolder>& dh,
					int sidx,int dim0sz,int dim1sz);
			/*Use for a block of data, eg the result of stepout.
			  dim0sz and dim1sz are usually nrinl and nrcrl.
			  Only 1 valueseries is used, the one at index sidx.
			  type_=1 */

			~DataHolderArray();

    void		set(int i0,int i1,int i2,float val) override;
    float		get(int i0,int i1,int i2) const override;
    const Array3DInfo&	info() const override		{ return info_; }

    void		getAll(float*) const override;
    void		getAll(ValueSeries<float>& vs) const override;

protected:

    Array3DInfoImpl	info_;
    ObjectSet<DataHolder> dh_;
    char		type_;
    int			seriesidx_;
};

} // namespace Attrib
