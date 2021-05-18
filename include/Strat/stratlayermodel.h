#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"

#include "elasticpropsel.h"
#include "propertyref.h"

class od_istream;
class od_ostream;
class uiStratLayerModel;

namespace Strat
{
class Layer;
class LayerSequence;
class UnitRef;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.

 */

mExpClass(Strat) LayerModel
{
public:

				LayerModel();
				LayerModel( const LayerModel& lm )
							{ *this = lm; }
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const { return seqs_.isEmpty(); }
    bool			isValid() const;
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence(int idx)	{ return *seqs_[idx]; }
    const LayerSequence&	sequence(int idx) const { return *seqs_[idx]; }
    Interval<float>		zRange() const;

    void			setEmpty();
    LayerSequence&		addSequence();
    LayerSequence&		addSequence(const LayerSequence&);
				//!< Does a match of props
    void			removeSequence(int);

    PropertyRefSelection&	propertyRefs()		{ return proprefs_; }
    const PropertyRefSelection& propertyRefs() const	{ return proprefs_; }
    void			prepareUse() const;

    void			setElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const
				{ return elasticpropsel_; }

    const RefTree&		refTree() const;

    bool			readHeader(od_istream&,PropertyRefSelection&,
					   int& nrseq,bool& mathpreserve);
    bool			read(od_istream&);
    bool			write(od_ostream&,int modnr=0,
					bool mathpreserve=false) const;

    static const char*		sKeyNrSeqs()	{ return "Nr Sequences"; }
    static FixedString		defSVelStr()	{ return "DefaultSVel"; }

protected:

    ObjectSet<LayerSequence>	seqs_;
    PropertyRefSelection	proprefs_;
    ElasticPropSelection	elasticpropsel_;

};


mExpClass(Strat) LayerModelProvider
{
public:

    virtual		~LayerModelProvider()	{}

    virtual LayerModel& getCurrent()		= 0;
    virtual LayerModel& getEdited(bool)		= 0;

    const LayerModel&	getCurrent() const
			{ return const_cast<LayerModelProvider*>(this)
						->getCurrent(); }
    const LayerModel&	getEdited( bool yn ) const
			{ return const_cast<LayerModelProvider*>(this)
						->getEdited(yn); }
};


mExpClass(Strat) LayerModelProviderImpl : public LayerModelProvider
{ mODTextTranslationClass(LayerModelProviderImpl)
public:

			LayerModelProviderImpl();
			~LayerModelProviderImpl();

    LayerModel&		getCurrent() override;
    LayerModel&		getEdited(bool yn) override;

private:

    void		setUseEdited(bool yn);
    void		setEmpty();
    void		setBaseModel(Strat::LayerModel*);
    void		resetEditing();
    void		initEditing();

    LayerModel*		modl_;
    LayerModel*		modled_ = nullptr;
    LayerModel*		curmodl_;

    friend class ::uiStratLayerModel;

};

}; // namespace Strat

