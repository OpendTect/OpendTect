#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          June 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "dbkey.h"
#include "ranges.h"
#include "statruncalc.h"

class uiParent;
class BinnedValueSet;
class BufferStringSet;
namespace Attrib { class EngineMan; class DescSet; }

/*! \brief FingerPrint Attribute parameters calculator */

mExpClass(uiAttributes) calcFingParsObject
{ mODTextTranslationClass(calcFingParsObject);
public:
			calcFingParsObject(uiParent*);
			~calcFingParsObject();
    void		setUserRefList( BufferStringSet* refset )
							{ reflist_ = refset; }
    void		setDescSet( Attrib::DescSet* ds ) { attrset_ = ds; }
    void		setWeights( TypeSet<int> wgs )	{ weights_ = wgs; }
    void		setRanges(TypeSet<Interval<float> > rg) {ranges_ = rg;}
    void		setValues( TypeSet<float> vals ){ values_ = vals; }
    void		setRgRefPick( const DBKey& id ) { rgpickset_ = id; }
    void		setRgRefType( int type )	{ rgreftype_ = type; }
    void		setValStatsType( Stats::Type typ ) { statstype_ = typ; }

    TypeSet<int>	getWeights() const		{ return weights_; }
    TypeSet<float>	getValues() const		{ return values_; }
    TypeSet< Interval<float> >	getRanges() const	{ return ranges_; }
    DBKey		getRgRefPick() const		{ return rgpickset_; }
    int			getRgRefType() const		{ return rgreftype_; }

    void		clearValues()			{ values_.erase(); }
    void		clearRanges()			{ ranges_.erase(); }
    void		clearWeights()			{ weights_.erase(); }

    BinnedValueSet*	createRangesBinIDSet() const;
    void		setValRgSet(BinnedValueSet*,bool);
    bool		computeValsAndRanges();
    static uiString	emTxt()	    { return tr("Cannot create 2D random "
					    "pickset to compute the ranges:"); }

protected:

    void		findDataSetID(DBKey&) const;
    Attrib::EngineMan*	createEngineMan();
    void		extractAndSaveValsAndRanges();
    void		saveValsAndRanges(const TypeSet<float>&,
					  const TypeSet< Interval<float> >&);
    void		fillInStats(BinnedValueSet*,
				ObjectSet< Stats::RunCalc<float> >&,
				Stats::Type) const;

    BufferStringSet*	reflist_;
    Attrib::DescSet*	attrset_;
    Stats::Type		statstype_;
    TypeSet<float>	values_;
    TypeSet<int>	weights_;
    TypeSet< Interval<float> > ranges_;
    ObjectSet<BinnedValueSet> posset_;
    uiParent*		parent_;
    DBKey		rgpickset_;
    int			rgreftype_;
    static void		create2DRandPicks(const DBKey& dsetid,
					  BinnedValueSet* rangesset);

};
