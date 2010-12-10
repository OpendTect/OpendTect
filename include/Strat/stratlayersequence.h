#ifndef stratlayersequence_h
#define stratlayersequence_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Oct 2010
 RCS:		$Id: stratlayersequence.h,v 1.6 2010-12-10 14:30:56 cvsbert Exp $
________________________________________________________________________


-*/

#include "stratlayer.h"
#include "propertyref.h"
class AIModel;

namespace Strat
{
class Layer;
class Level;
class UnitRef;
class RefTree;

/*!\brief A sequence of layers.

  You can provide a PropertyRefSelection* to give meaning to the values in the Layers.

 */

mClass LayerSequence
{
public:

			LayerSequence(const PropertyRefSelection* prs=0);
			LayerSequence( const LayerSequence& ls )
			    			{ *this = ls; }
    virtual		~LayerSequence();
    LayerSequence&	operator =(const LayerSequence&);
    bool		isEmpty() const		{ return layers_.isEmpty(); }

    int			size() const		{ return layers_.size(); }
    ObjectSet<Layer>&	layers()		{ return layers_; }
    const ObjectSet<Layer>& layers() const	{ return layers_; }

    float		startDepth() const	{ return z0_; }
    void		setStartDepth( float z ) { z0_ = z; }

    const PropertyRefSelection& propertyRefs() const	{ return props_; }
    void		setPropertyRefs( const PropertyRefSelection& prs )
							{ props_ = prs; }

    void		getLayersFor( const UnitRef* ur, ObjectSet<Layer>& lys )
			{ return getLayersFor(ur,(ObjectSet<const Layer>&)lys);}
    void		getLayersFor(const UnitRef*,
	    			     ObjectSet<const Layer>&) const;
    const RefTree&	refTree() const;

    void		prepareUse() const ;	//!< needed after changes

    int			indexOf(const Level&,int startsearchat=0) const;
    AIModel*		getAIModel(int velidx,int denidx,
	    			   bool isvel=true,bool isden=true) const;
    			// if !isvel, then sonic (i.e. 1/vel)
    			// if !isden, then AI

protected:

    ObjectSet<Layer>	layers_;
    float		z0_;
    PropertyRefSelection props_;

};


}; // namespace Strat

#endif
