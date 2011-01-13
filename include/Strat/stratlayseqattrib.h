#ifndef stratlayseqattrib_h
#define stratlayseqattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id: stratlayseqattrib.h,v 1.1 2011-01-13 14:52:13 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "bufstringset.h"
class PropertyRef;


namespace Strat
{

/*!\brief attrib to extract from layer sequences

  Rather than attaching everything to the UnitRefs, PropertyRefs etc., we simply
  work with strings. When actual evaluation needs to be done, construct a
  LaySeqAttribCalc object.
 
 */

mClass LaySeqAttrib : public NamedObject
{
public:

    			LaySeqAttrib( const PropertyRef& p, const char* nm=0 )
			    : prop_(p), NamedObject(nm)		{}

    const PropertyRef&	prop_;
    BufferString	stat_;
    BufferStringSet	units_;
    BufferStringSet	lithos_;

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
    LaySeqAttrib*	attr( const PropertyRef* r )	   { return gtAttr(r); }
    const LaySeqAttrib*	attr( const PropertyRef* r ) const { return gtAttr(r); }

protected:

    LaySeqAttrib*	gtAttr(const char*) const;
    LaySeqAttrib*	gtAttr(const PropertyRef*) const;

};


}; // namespace Strat

#endif
