#ifndef stratsinglaygen_h
#define stratsinglaygen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratsinglaygen.h,v 1.6 2012/01/24 16:40:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "stratlaygen.h"


namespace Strat
{
class LeafUnitRef;
class Content;

/*!\brief Layer generator based on Leaf Unit */

mClass SingleLayerGenerator : public LayerGenerator
{
public:

    			SingleLayerGenerator(const LeafUnitRef* ur=0);
    			~SingleLayerGenerator()	{}

    const LeafUnitRef&	unit() const;
    void		setUnit( const LeafUnitRef* ur ) { unit_ = ur; }
    const Content&	content() const			{ return *content_; }
    void		setContent( const Content& c )	{ content_ = &c; }

    bool		isEmpty() const		{ return props_.isEmpty(); }
    PropertySet&	properties()		{ return props_; }
    const PropertySet&	properties() const	{ return props_; }

    virtual bool	reset() const;
    virtual const char*	errMsg() const		{ return errmsg_.buf(); }

    mDefLayerGeneratorFns(SingleLayerGenerator,"Single layer");

protected:

    const LeafUnitRef*	unit_;
    PropertySet		props_;
    const Content*	content_;
    mutable BufferString errmsg_;

};


}; // namespace Strat

#endif
