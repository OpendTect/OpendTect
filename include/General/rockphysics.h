#ifndef rockphysics_h
#define rockphysics_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Feb 2012
 RCS:		$Id$
________________________________________________________________________


-*/

#include "generalmod.h"
#include "propertyref.h"
#include "repos.h"
class MathProperty;
class ascistream;
class ascostream;


/*!\brief Ref Data for a (usually petrophysical) property.

We prepare for many variants of the name as is not uncommon in practice
(Density, Den, Rho, RhoB, ... you know the drill). The names will be unique
- case insensitive, in the Set. Hence, identity is established case insensitive.
Aliases are matched with a GlobExpr, so you can add with wildcards and the like.

 */


namespace RockPhysics
{

mExpClass(General) Formula : public NamedObject
{
public:

    typedef PropertyRef::StdType PropType;

			Formula( PropType t, const char* nm=0 )
			: NamedObject(nm)
			, type_(t)		{}
    static Formula*	get(const IOPar&);	//!< returns null if bad IOPar
			~Formula()		{ deepErase(constdefs_); }
			Formula( const Formula& f ) { *this = f; }
    Formula&		operator =(const Formula&);
    inline bool		operator ==( const Formula& pr ) const
			{ return name() == pr.name(); }
    inline bool		operator !=( const Formula& pr ) const
			{ return name() != pr.name(); }

    inline bool		hasPropType( PropType t ) const
						{ return type_ == t; }

    mExpClass(General) ConstDef : public NamedObject
    {
    public:
			ConstDef( const char* nm )
			    : NamedObject(nm)
			    , typicalrg_(mUdf(float),mUdf(float))
			    , defaultval_(mUdf(float))	{}
	BufferString	desc_;
	Interval<float>	typicalrg_;
	float defaultval_;
    };
    mExpClass(General) VarDef : public NamedObject
    {
    public:
			VarDef( const char* nm, PropType t )
			    : NamedObject(nm)
			    , type_(t)			{}
	BufferString	desc_;
	PropType	type_;
	BufferString	unit_;
    };

    PropType		type_;
    BufferString	def_;
    BufferString	desc_;
    ObjectSet<ConstDef>	constdefs_;
    ObjectSet<VarDef>	vardefs_;
    Repos::Source	src_;
    BufferString	formulaunit_;
    BufferString	outputunit_;

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    bool		setDef(const char*); // Will add var- and constdefs
    MathProperty*	getProperty(const PropertyRef* pr=0) const;

};


mExpClass(General) FormulaSet : public ObjectSet<const Formula>
{
public:
    			~FormulaSet()
			{ deepErase( *(ObjectSet<Formula>*)this ); }
    int			getIndexOf(const char*) const;
    void		getRelevant(PropertyRef::StdType,
	    			    BufferStringSet&) const;

    const Formula*	get( const char* nm ) const
			{
			    const int idxof = getIndexOf( nm );
			    return idxof<0 ? 0 : (*this)[idxof];
			}

    bool		save(Repos::Source) const;

    void		readFrom(ascistream&);
    bool		writeTo(ascostream&) const;

};



} // namespace RockPhysics

mGlobal(General) const RockPhysics::FormulaSet& ROCKPHYSFORMS();
mGlobal(General) inline RockPhysics::FormulaSet& eROCKPHYSFORMS()
{ return const_cast<RockPhysics::FormulaSet&>( ROCKPHYSFORMS() ); }


#endif

