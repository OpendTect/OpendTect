#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"

#include "ailayer.h"
#include "propertyref.h"

class ElasticPropSelection;

namespace Strat
{
class Layer;
class Level;
class RefTree;
class UnitRef;

/*!\brief A sequence of layers.

  You can provide a PropertySelection* to give meaning to the values in
  the Layers.

 */

mExpClass(Strat) LayerSequence
{
public:

			LayerSequence(const PropertyRefSelection* =nullptr);
			LayerSequence(const LayerSequence&);
    virtual		~LayerSequence();

    LayerSequence&	operator =(const LayerSequence&);

    bool		isEmpty() const;
    void		setEmpty();

    int			size() const;
    ObjectSet<Layer>&	layers()		{ return layers_; }
    const ObjectSet<Layer>& layers() const	{ return layers_; }
    int			layerIdxAtZ(float) const; //!< returns -1 if outside
    int			nearestLayerIdxAtZ(float z) const;
						//!< returns -1 only if empty

    float		startDepth() const	{ return z0_; }
    float		overburdenVelocity() const;
    void		setStartDepth( float z ) { z0_ = z; prepareUse(); }
    void		setOverburdenVelocity(float vel);
    Interval<float>	zRange() const;
    Interval<float>	propRange(int) const;
    void		setXPos(float);

    PropertyRefSelection& propertyRefs()	{ return props_; }
    const PropertyRefSelection& propertyRefs() const	{ return props_; }

    void		getLayersFor( const UnitRef* ur,
				      ObjectSet<Layer>& lys )
			{return getLayersFor(ur,(ObjectSet<const Layer>&)lys);}
    void		getLayersFor(const UnitRef*,
				     ObjectSet<const Layer>&) const;
    void		getSequencePart(const Interval<float>& depthrg,
					bool cropfirstlast,
					LayerSequence&) const;
			//!< cropfirstlast updates thicknesses of first and last
			//!< layers to exactly match the window
    const RefTree&	refTree() const;

    void		prepareUse() const ;	//!< needed after changes

			// Following will need to actually find the level
    int			indexOf(const Level&,int startsearchat=0) const;
			//!< may return -1 for not found
    float		depthOf(const Level&,float notfoundval=0.f) const;
			//!< will return 0 if lvl not found

			// Following will give the position of the level
			// ... even if not present
    int			positionOf(const Level&) const;
			//!< may return size() (below last layer)
			//!< only returns -1 if sequence is empty
    float		depthPositionOf(const Level&,
					float notfoundval=0.f) const;

protected:

    void		adjustLayers(float startz);

    ObjectSet<Layer>	layers_;
    float		z0_ = 0.f;
    PropertyRefSelection props_;

public:
    void		setStartDepthOnly( float z ) { z0_ = z; }
    void		setStartDepthAndAdjust(float z);


};


mExpClass(Strat) LayerSequenceOv : public LayerSequence
{
public:
			LayerSequenceOv(const PropertyRefSelection* =nullptr);
			LayerSequenceOv(const LayerSequenceOv&);
			LayerSequenceOv(const LayerSequence&);
			~LayerSequenceOv();

    LayerSequence&	operator =(const LayerSequenceOv&);
    LayerSequence&	operator =(const LayerSequence&);

    float		overburdenVelocity_() const	{ return velabove_; }
    void		setOverburdenVelocity_(float vel);

private:

    float		velabove_ = 2000.f;
};

} // namespace Strat
