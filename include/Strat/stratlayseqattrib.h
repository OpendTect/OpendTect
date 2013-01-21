#ifndef stratlayseqattrib_h
#define stratlayseqattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "stratmod.h"
#include "namedobj.h"
#include "enums.h"
#include "bufstringset.h"
class PropertyRef;
class IOPar;


namespace Strat
{
class LaySeqAttribSet;

/*!\brief attrib to extract from layer sequences

  Rather than attaching everything to the UnitRefs, PropertyRefs etc., we simply
  work with strings. When actual evaluation needs to be done, construct a
  LaySeqAttribCalc object.
 
 */

mExpClass(Strat) LaySeqAttrib : public NamedObject
{
public:

    enum Transform	{ Pow, Log, Exp };
    			DeclareEnumUtils(Transform)

    			LaySeqAttrib( LaySeqAttribSet& s,const PropertyRef& p,
				      const char* nm=0 )
			    : NamedObject(nm)
			    , set_(&s), prop_(p)
			    , islocal_(false)
			    , transform_(Pow)
			    , transformval_(mUdf(float))	{}

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
{
public:

    			LaySeqAttribSet( const char* nm=0 )
			    : NamedObject(nm)
			    , ManagedObjectSet<LaySeqAttrib>(false)	{}

    LaySeqAttrib&	attr( int idx )		{ return *(*this)[idx]; }
    const LaySeqAttrib&	attr( int idx ) const	{ return *(*this)[idx]; }
    LaySeqAttrib*	attr( const char* nm )	   	{ return gtAttr(nm); }
    const LaySeqAttrib*	attr( const char* nm ) const	{ return gtAttr(nm); }

    void		getFrom(const IOPar&);
    void		putTo(IOPar&) const;
    bool		getFrom(std::istream&);
    bool		putTo(std::ostream&) const;

protected:

    LaySeqAttrib*	gtAttr(const char*) const;

};


}; // namespace Strat

#endif

