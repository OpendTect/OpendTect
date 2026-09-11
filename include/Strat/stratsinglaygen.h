#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"

#include "ptrman.h"
#include "stratlaygen.h"


namespace Strat
{
class LeafUnitRef;
class Content;
class SharedFormula;

/*!\brief Layer generator based on Leaf Unit */

mExpClass(Strat) SingleLayerGenerator : public LayerGenerator
{
public:

			SingleLayerGenerator(const LeafUnitRef* =nullptr);
			SingleLayerGenerator(const SingleLayerGenerator&);
    SingleLayerGenerator& operator =(const SingleLayerGenerator&);
			~SingleLayerGenerator();

    bool		canBeCloned() const override	{ return true; }
    const LeafUnitRef&	unit() const;
    void		setUnit( const LeafUnitRef* ur ) { unit_ = ur; }
    const Content&	content() const			{ return *content_; }
    void		setContent( const Content& c )	{ content_ = &c; }

    bool		isEmpty() const		{ return props_.isEmpty(); }
    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }

    bool		reset() const override;
    uiString		errMsg() const override		{ return errmsg_; }

    mDefLayerGeneratorFns(SingleLayerGenerator,"Single layer");

protected:

    LayerGenerator*	createClone() const override;
    const LeafUnitRef*	unit_;
    PropertySet		props_;
    const Content*	content_;
    mutable uiString	errmsg_;
    mutable RefObjectSet<SharedFormula> sharedforms_;

    ConstRefMan<SharedFormula> getSharedForm(int iprop) const;

};


} // namespace Strat
