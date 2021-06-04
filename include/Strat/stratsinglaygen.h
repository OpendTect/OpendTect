#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
________________________________________________________________________

-*/

#include "stratmod.h"
#include "stratlaygen.h"


namespace Strat
{
class LeafUnitRef;
class Content;

/*!\brief Layer generator based on Leaf Unit */

mExpClass(Strat) SingleLayerGenerator : public LayerGenerator
{
public:

    			SingleLayerGenerator(const LeafUnitRef* ur=0);
    			SingleLayerGenerator(const SingleLayerGenerator&);
    			~SingleLayerGenerator()	{}

    virtual bool	canBeCloned() const		{ return true; }
    const LeafUnitRef&	unit() const;
    void		setUnit( const LeafUnitRef* ur ) { unit_ = ur; }
    const Content&	content() const			{ return *content_; }
    void		setContent( const Content& c )	{ content_ = &c; }

    bool		isEmpty() const		{ return props_.isEmpty(); }
    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }

    virtual bool	reset() const;
    virtual uiString	errMsg() const		{ return errmsg_; }

    mDefLayerGeneratorFns(SingleLayerGenerator,"Single layer");

protected:

    virtual LayerGenerator* createClone() const;
    const LeafUnitRef*	unit_;
    PropertySet		props_;
    const Content*	content_;
    mutable uiString	errmsg_;

};


}; // namespace Strat

