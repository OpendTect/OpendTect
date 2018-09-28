#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"

#include "elasticpropsel.h"
#include "propertyref.h"
#include "stratlayersequence.h"
#include "manobjectset.h"
#include "od_iosfwd.h"


namespace Strat
{
class Layer;
class UnitRef;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.

 */

mExpClass(Strat) LayerModel
{ mIsContainer( LayerModel, ObjectSet<LayerSequence>, seqs_ )
public:

				LayerModel();
				LayerModel( const LayerModel& lm )
							{ *this = lm; }
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const;
    bool			isValid() const;
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence( int idx )	  { return *seqs_[idx];}
    const LayerSequence&	sequence( int idx ) const { return *seqs_[idx];}
    int				nrLayers() const;
    Interval<float>		zRange() const;

    void			setEmpty();
    LayerSequence&		addSequence();
    LayerSequence&		addSequence(const LayerSequence&);
					//!< will not preserve Math!
    void			removeSequence(int);

    PropertyRefSelection&	propertyRefs()		{ return proprefs_; }
    const PropertyRefSelection&	propertyRefs() const	{ return proprefs_; }
    void			prepareUse() const;

    void			setElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const {return elasticpropsel_;}

    const RefTree&		refTree() const;

    bool			read(od_istream&,bool loadinto,int addeach=1);
    bool			write(od_ostream&,int modnr=0,
					bool mathpreserve=false) const;

    static const char*		sKeyNrSeqs()	{ return "Nr Sequences"; }
    static FixedString          defSVelStr()	{ return "DefaultSVel"; }

protected:

    PropertyRefSelection	proprefs_;
    ElasticPropSelection	elasticpropsel_;

};

mDefContainerSwapFunction( Strat, LayerModel )


/*!\brief set of related LayerModels that are edits of an original
  (the first one). Each model has a description. The suite is never empty. */

mExpClass(Strat) LayerModelSuite : public CallBacker
{ mIsContainer( LayerModelSuite, ManagedObjectSet<LayerModel>, mdls_ )
public:

			LayerModelSuite();
    virtual		~LayerModelSuite()	{}

    size_type		size() const		{ return mdls_.size();}
    LayerModel&		get( idx_type i )	{ return *mdls_.get(i); }
    const LayerModel&	get( idx_type i ) const	{ return *mdls_.get(i); }
    LayerModel&		baseModel()		{ return *mdls_.first(); }
    const LayerModel&	baseModel() const	{ return *mdls_.first(); }

    BufferString	desc( idx_type i ) const { return descs_.get(i); }
    uiString		uiDesc( idx_type i ) const { return uidescs_.get(i); }
    void		setDesc(int,const char*,const uiString&);

    idx_type		curIdx() const		{ return curidx_; }
    LayerModel&		getCurrent()		{ return get( curidx_ ); }
    const LayerModel&	getCurrent() const	{ return get( curidx_ ); }
    void		setCurIdx(idx_type);

    void		addModel(const char*,const uiString&);
    void		removeModel(idx_type);

    bool		curIsEdited() const	{ return curidx_ > 0; }
    bool		hasEditedData() const;
    void		clearEditedData();
    void		prepareEditing();

    Notifier<LayerModelSuite>		curChanged;
    CNotifier<LayerModelSuite,bool>	editingChanged;
			//!< passes whether there was edited data before change

protected:

    idx_type		curidx_			= 0;
    BufferStringSet	descs_;
    uiStringSet		uidescs_;

public:

    void		setEmpty();		//!< keeps empty base model
    void		setBaseModel(LayerModel*); //!< will clear editing

};

mDefContainerSwapFunction( Strat, LayerModelSuite )


}; // namespace Strat
