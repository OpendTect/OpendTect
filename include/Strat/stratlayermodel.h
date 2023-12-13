#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"

#include "elasticpropsel.h"
#include "stattype.h"

class od_istream;
class od_ostream;
class TaskRunner;

namespace Strat
{
class Layer;
class LayerSequence;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.

 */

mExpClass(Strat) LayerModel
{ mIsContainer( LayerModel, ObjectSet<LayerSequence>, seqs_ )
  mODTextTranslationClass(LayerModel)
public:

				LayerModel();
				LayerModel(const LayerModel&);
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const;
    bool			isValid() const;
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence( int idx )	  { return *seqs_[idx];}
    const LayerSequence&	sequence( int idx ) const { return *seqs_[idx];}
    int				nrLayers() const;
    Interval<float>		zRange() const;
    float			startDepth(Stats::Type=Stats::Average) const;
    float			overburdenVelocity(
					Stats::Type=Stats::Average) const;

    void			setEmpty();
    LayerSequence&		addSequence();
    LayerSequence&		addSequence(const LayerSequence&);
				//!< Does a match of props
    void			removeSequence(int);
    void			append(const LayerModel&);
				//!< Does a match of props

    PropertyRefSelection&	propertyRefs()		{ return proprefs_; }
    const PropertyRefSelection& propertyRefs() const	{ return proprefs_; }
    void			prepareUse() const;

    void			setElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const
				{ return elasticpropsel_; }

    const RefTree&		refTree() const;

    bool			readHeader(od_istream&,PropertyRefSelection&,
					   int& nrseq,bool& mathpreserve);

    bool			read(od_istream&,int start,int step,
				     uiString&,TaskRunner* =nullptr,
				     float startdepth=mUdf(float),
				     float abovevel=mUdf(float));
    bool			write(od_ostream&,int modnr=0,
					bool mathpreserve=false) const;

    static const char*		sKeyNrSeqs()	{ return "Nr Sequences"; }
    static StringView		defSVelStr()	{ return "DefaultSVel"; }

protected:

    friend class		LayerModelReader;

    PropertyRefSelection	proprefs_;
    ElasticPropSelection	elasticpropsel_;

};

mDefContainerSwapFunction( Strat, LayerModel )


/*!\brief set of related LayerModels that are edits of an original
  (the first one). Each model has a description. The suite is never empty. */

mExpClass(Strat) LayerModelSuite : public CallBacker
{
public:

			LayerModelSuite();
    virtual		~LayerModelSuite();

    int			size() const		{ return mdls_.size(); }
    LayerModel&		get( int idx )		{ return *mdls_.get(idx); }
    const LayerModel&	get( int idx ) const	{ return *mdls_.get(idx); }
    LayerModel&		baseModel()		{ return *mdls_.first(); }
    const LayerModel&	baseModel() const	{ return *mdls_.first(); }

    BufferString	desc( int idx ) const	{ return descs_.get(idx); }
    uiString		uiDesc( int idx ) const { return uidescs_.get(idx); }
    void		setDesc(int,const char*,const uiString&);

    int			curIdx() const		{ return curidx_; }
    LayerModel&		getCurrent()		{ return get( curidx_ ); }
    const LayerModel&	getCurrent() const	{ return get( curidx_ ); }
    void		setCurIdx(int);

    void		addModel(const char*,const uiString&);
    void		removeModel(int);
    void		setElasticPropSel(const ElasticPropSelection&);

    bool		curIsEdited() const	{ return curidx_ > 0; }
    bool		hasEditedData() const;
    void		clearEditedData();
    void		prepareEditing();
    void		touch(int ctyp=-1)	{ modelChanged.trigger(ctyp); }

    Notifier<LayerModelSuite> curChanged;
    CNotifier<LayerModelSuite,bool> baseChanged;
			//!< passes whether there was base data before change
    CNotifier<LayerModelSuite,bool> editingChanged;
			//!< passes whether there was edited data before change
    CNotifier<LayerModelSuite,int> modelChanged;
			//!< The content of the current model was changed

private:

    ManagedObjectSet<LayerModel> mdls_;
    int			curidx_ = 0;
    BufferStringSet	descs_;
    uiStringSet		uidescs_;

public:

    void		setEmpty();
			//!< keeps empty base model
    void		setBaseModel(LayerModel*,bool setascurrent=false);
			//!< will clear editing

};

} // namespace Strat
