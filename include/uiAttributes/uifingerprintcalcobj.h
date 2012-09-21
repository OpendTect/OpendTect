#ifndef uifingerprintcalcobj_h
#define uifingerprintcalcobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          June 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "multiid.h"
#include "ranges.h"
#include "statruncalc.h"

class uiParent;
class BinIDValueSet;
class BufferStringSet;
namespace Attrib { class EngineMan; class DescSet; }

using namespace Attrib;

/*! \brief FingerPrint Attribute parameters calculator */

mClass(uiAttributes) calcFingParsObject
{
public:
    			calcFingParsObject(uiParent*);
			~calcFingParsObject();
    void		setUserRefList( BufferStringSet* refset )
    							{ reflist_ = refset; }
    void		setDescSet( DescSet* ds )	{ attrset_ = ds; }
    void                setWeights( TypeSet<int> wgs )  { weights_ = wgs; }
    void		setRanges(TypeSet<Interval<float> > rg) {ranges_ = rg;}
    void		setValues( TypeSet<float> vals ){ values_ = vals; }
    void		setRgRefPick(const char* pickid){ rgpickset_ = pickid; }
    void		setRgRefType( int type )	{ rgreftype_ = type; }
    void		setValStatsType( int typ )	{ statstype_ = typ; }
    
    TypeSet<int>	getWeights() const		{ return weights_; }
    TypeSet<float>	getValues() const		{ return values_; }
    TypeSet< Interval<float> >	getRanges() const	{ return ranges_; }
    BufferString	getRgRefPick() const		{ return rgpickset_; }
    int			getRgRefType() const		{ return rgreftype_; }

    void		clearValues()			{ values_.erase(); }
    void		clearRanges()			{ ranges_.erase(); }
    void		clearWeights()			{ weights_.erase(); }
    
    BinIDValueSet*	createRangesBinIDSet() const;
    void		setValRgSet(BinIDValueSet*,bool);
    bool		computeValsAndRanges();
protected:
    
    void		findLineSetID(MultiID&) const;
    EngineMan*          createEngineMan();
    void                extractAndSaveValsAndRanges();
    void                saveValsAndRanges(const TypeSet<float>&,
	    				  const TypeSet< Interval<float> >&);
    void		fillInStats(BinIDValueSet*,
				ObjectSet< Stats::RunCalc<float> >&,
				Stats::Type) const;

    BufferStringSet*	reflist_;
    DescSet*		attrset_;
    int			statstype_;
    TypeSet<float>	values_;
    TypeSet<int>	weights_;
    TypeSet< Interval<float> > ranges_;
    ObjectSet<BinIDValueSet> posset_;
    uiParent*		parent_;
    BufferString	rgpickset_;
    int			rgreftype_;

};

#endif

