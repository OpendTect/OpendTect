#ifndef stratsinglaygen_h
#define stratsinglaygen_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratsinglaygen.h,v 1.3 2010-10-28 15:11:56 cvsbert Exp $
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

    virtual bool	reset() const;
    virtual const char*	errMsg() const		{ return errmsg_.buf(); }

    mDefLayerGeneratorFns(SingleLayerGenerator,"Single layer");

protected:

    const LeafUnitRef*	unit_;
    PropertySet		props_;
    mutable BufferString errmsg_;

};


}; // namespace Strat

#endif
