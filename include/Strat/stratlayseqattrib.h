#ifndef stratlayseqattrib_h
#define stratlayseqattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id: stratlayseqattrib.h,v 1.2 2011-01-14 14:44:09 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
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

mClass LaySeqAttrib : public NamedObject
{
public:

    			LaySeqAttrib( LaySeqAttribSet& s,const PropertyRef& p,
				      const char* nm=0 )
			    : set_(&s), prop_(p), NamedObject(nm)	{}

    const PropertyRef&	prop_;
    BufferString	stat_;
    BufferStringSet	units_;
    BufferStringSet	lithos_;

    static const char*	sKeyStats()		{ return "Statistics"; }
    static const char*	sKeyUnits()		{ return "Units"; }
    static const char*	sKeyLithos()		{ return "Lithologies"; }

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


mClass LaySeqAttribSet : public NamedObject
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
