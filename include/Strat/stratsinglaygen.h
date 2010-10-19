#ifndef stratsinglaygen_h
#define stratsinglaygen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratsinglaygen.h,v 1.2 2010-10-19 15:13:30 cvsbert Exp $
________________________________________________________________________

-*/

#include "stratlayseqgendesc.h"
#include "property.h"


namespace Strat
{
class LeafUnitRef;

/*!\brief Layer generator based on Leaf Unit */

mClass SingleLayerGenerator : public LayerGenerator
{
public:

    			SingleLayerGenerator(const LeafUnitRef* ur=0);
    			~SingleLayerGenerator()	{}

    const LeafUnitRef&	unit() const;
    void		setUnit( const LeafUnitRef* ur ) { unit_ = ur; }

    bool		isEmpty() const		{ return props_.isEmpty(); }
    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }
    void		getPropertySelection(PropertyRefSelection&) const;

    mDefLayerGeneratorFns(SingleLayerGenerator,"Single layer");

protected:

    const LeafUnitRef*	unit_;
    PropertySet		props_;

};


}; // namespace Strat

#endif
