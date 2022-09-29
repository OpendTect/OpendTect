#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "namedobj.h"
#include "enums.h"
#include "uistring.h"
#include "bufstringset.h"
#include "od_iosfwd.h"
class PropertyRef;


namespace Strat
{
class LaySeqAttribSet;

/*!\brief attrib to extract from layer sequences

  Rather than attaching everything to the UnitRefs, PropertyRefs etc., we simply
  work with strings. When actual evaluation needs to be done, construct a
  LaySeqAttribCalc object.

 */

mExpClass(Strat) LaySeqAttrib : public NamedObject
{ mODTextTranslationClass(LaySeqAttrib);
public:

    enum Transform	{ Pow, Log, Exp };
			mDeclareEnumUtils(Transform)

			LaySeqAttrib( LaySeqAttribSet& s,const PropertyRef& p,
				      const char* nm=0 );
			~LaySeqAttrib();
    const PropertyRef&	prop_;
    bool		islocal_;
    BufferString	stat_; // either Stats::Type or Stats::UpscaleType

    // non-local only
    BufferStringSet	units_;
    BufferStringSet	liths_;

    Transform		transform_;
    float		transformval_;
    inline bool		hasTransform() const
			{ return !mIsUdf(transformval_); }

    static const char*	sKeyIsLocal()		{ return "Local"; }
    static const char*	sKeyStats()		{ return "Statistics"; }
    static const char*	sKeyUnits()		{ return "Units"; }
    static const char*	sKeyLithos()		{ return "Lithologies"; }
    static const char*	sKeyTransform()		{ return "Transform"; }

    LaySeqAttribSet&	attrSet()		{ return *set_; }
    const LaySeqAttribSet& attrSet() const	{ return *set_; }
    void		setAttrSet( LaySeqAttribSet& s ) { set_ = &s; }

protected:

    LaySeqAttribSet*	set_;

};


/*!\brief attribs to extract from layer sequences

  Rather than attaching everything to the UnitRefs, PropertyRefs etc., we simply
  work with strings. When actual evaluation needs to be done, the
  LaySeqAttribCalc will step in.

 */


mExpClass(Strat) LaySeqAttribSet : public NamedObject
		       , public ManagedObjectSet<LaySeqAttrib>
{ mODTextTranslationClass(LaySeqAttribSet);
public:

			LaySeqAttribSet(const char* nm=0);
			~LaySeqAttribSet();

    LaySeqAttrib&	attr( int idx )		{ return *(*this)[idx]; }
    const LaySeqAttrib&	attr( int idx ) const	{ return *(*this)[idx]; }
    LaySeqAttrib*	attr( const char* nm )		{ return gtAttr(nm); }
    const LaySeqAttrib*	attr( const char* nm ) const	{ return gtAttr(nm); }

    void		getFrom(const IOPar&);
    void		putTo(IOPar&) const;
    bool		getFrom(od_istream&);
    bool		putTo(od_ostream&) const;

protected:

    LaySeqAttrib*	gtAttr(const char*) const;

};


} // namespace Strat
