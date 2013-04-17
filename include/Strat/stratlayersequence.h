#ifndef stratlayersequence_h
#define stratlayersequence_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Oct 2010
 RCS:		$Id$
________________________________________________________________________


-*/

#include "stratmod.h"
#include "ailayer.h"
#include "stratlayer.h"
#include "propertyref.h"

class ElasticPropSelection;

namespace Strat
{
class Layer;
class Level;
class UnitRef;
class RefTree;

/*!\brief A sequence of layers.

  You can provide a PropertyRefSelection* to give meaning to the values in the Layers.

 */

mExpClass(Strat) LayerSequence
{
public:

			LayerSequence(const PropertyRefSelection* prs=0);
			LayerSequence( const LayerSequence& ls )
			    			{ *this = ls; }
    virtual		~LayerSequence();
    LayerSequence&	operator =(const LayerSequence&);
    bool		isEmpty() const		{ return layers_.isEmpty(); }
    void		setEmpty()		{ layers_.setEmpty(); }

    int			size() const		{ return layers_.size(); }
    ObjectSet<Layer>&	layers()		{ return layers_; }
    const ObjectSet<Layer>& layers() const	{ return layers_; }
    int			layerIdxAtZ(float) const; //!< returns -1 if outside
    int			nearestLayerIdxAtZ(float z) const;
    						//!< returns -1 only if empty

    float		startDepth() const	{ return z0_; }
    void		setStartDepth( float z ) { z0_ = z; prepareUse(); }
    Interval<float>	zRange() const;

    PropertyRefSelection& propertyRefs() 	{ return props_; }
    const PropertyRefSelection& propertyRefs() const	{ return props_; }

    void		getLayersFor( const UnitRef* ur, ObjectSet<Layer>& lys )
			{ return getLayersFor(ur,(ObjectSet<const Layer>&)lys);}
    void		getLayersFor(const UnitRef*,
	    			     ObjectSet<const Layer>&) const;
    const RefTree&	refTree() const;

    void		prepareUse() const ;	//!< needed after changes

    			// Following will need to actually find the level
    int			indexOf(const Level&,int startsearchat=0) const;
    			//!< may return -1 for not found
    float		depthOf(const Level&) const;
    			//!< will return 0 if lvl not found

    			// Following will give the position of the level
    			// ... even if not present
    int			positionOf(const Level&) const;
    			//!< may return size() (below last layer)
			//!< only returns -1 if sequence is empty
    float		depthPositionOf(const Level&) const;

protected:

    ObjectSet<Layer>	layers_;
    float		z0_;
    PropertyRefSelection props_;

};


}; // namespace Strat

#endif

