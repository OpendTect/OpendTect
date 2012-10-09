#ifndef stratlayermodel_h
#define stratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id$
________________________________________________________________________


-*/

#include "stratlayersequence.h"
#include "elasticpropsel.h"


namespace Strat
{
class Layer;
class UnitRef;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.
 
 */

mClass LayerModel
{
public:

				LayerModel();
				LayerModel( const LayerModel& lm )
							{ *this = lm; }
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const	{ return seqs_.isEmpty(); }
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence( int idx )	  { return *seqs_[idx];}
    const LayerSequence& 	sequence( int idx ) const { return *seqs_[idx];}

    LayerSequence&		addSequence();
    void			setEmpty();

    PropertyRefSelection&	propertyRefs()		{ return props_; }
    const PropertyRefSelection&	propertyRefs() const	{ return props_; }
    void			prepareUse() const;

    void 			addElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const {return elasticpropsel_;}

    const RefTree&		refTree() const;

    bool			read(std::istream&);
    bool			write(std::ostream&,int modnr=0) const;
    static const char*		sKeyNrSeqs()		{return "Nr Sequences";}

protected:

    ObjectSet<LayerSequence>	seqs_;
    PropertyRefSelection	props_;
    ElasticPropSelection	elasticpropsel_;

};


mClass LayerModelProvider
{
public:

    virtual LayerModel&		get()	= 0;
    const LayerModel&	get() const
			{ return const_cast<LayerModelProvider*>(this)->get(); }
};


}; // namespace Strat

#endif
