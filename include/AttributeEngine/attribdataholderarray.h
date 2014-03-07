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

    void		set(int i0,int i1,int i2,float val);
    float		get(int i0,int i1,int i2) const;
    const Array3DInfo&	info() const		{ return info_; }

    virtual void	getAll(float*) const;
    virtual void	getAll(ValueSeries<float>& vs) const;

protected:

    Array3DInfoImpl	info_;
    ObjectSet<DataHolder> dh_;
    char		type_;
    int			seriesidx_;
};

} // namespace Attrib

#endif

