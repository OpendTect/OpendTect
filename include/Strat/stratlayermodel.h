#ifndef stratlayermodel_h
#define stratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id: stratlayermodel.h,v 1.3 2010-10-06 15:40:52 cvsbert Exp $
________________________________________________________________________


-*/

#include "stratlayer.h"

namespace Strat
{
class Layer;
class UnitRef;
class RefTree;

/*!\brief A model consisting of layers */

mClass LayerModel
{
public:

			LayerModel()
			    : z0_(0)		{}
    virtual		~LayerModel()		{ deepErase(layers_); }
    bool		isEmpty() const		{ return layers_.isEmpty(); }

    int			size() const		{ return layers_.size(); }
    ObjectSet<Layer>&	layers()		{ return layers_; }
    const ObjectSet<Layer>& layers() const	{ return layers_; }

    float		startZ() const		{ return z0_; }
    void		setStartZ( float z )	{ z0_ = z; }

    RefTree*		refTree()		{ return gtTree(); }
    const RefTree*	refTree() const		{ return gtTree(); }

    void		getLayersFor( const UnitRef* ur, ObjectSet<Layer>& lys )
			{ return getLayersFor(ur,(ObjectSet<const Layer>&)lys);}
    void		getLayersFor(const UnitRef*,
	    			     ObjectSet<const Layer>&) const;

    void		prepareUse();		//!< needed after changes

protected:

    ObjectSet<Layer>	layers_;
    float		z0_;

    RefTree*		gtTree() const;

};


}; // namespace Strat

#endif
