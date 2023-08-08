#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "multiid.h"


namespace Attrib
{

class DescSet;

/*!
\brief Manages attribute description set. 
*/

mExpClass(AttributeEngine) DescSetMan
{
public:

			DescSetMan(bool,DescSet* ads=0,bool destr_on_del=true );
			~DescSetMan();

    DescSet*		descSet()			{ return ads_; }
    const DescSet*	descSet() const			{ return ads_; }
    void		setDescSet(DescSet*);

    bool		is2D() const			{ return is2d_; }
    bool		isSaved() const          	{ return !unsaved_; }
    void		setSaved( bool yn=true ) const
			{ const_cast<DescSetMan*>(this)->unsaved_ = !yn; }
    bool&		unSaved()			{ return unsaved_; }
			//!< Added for convenience in UI building

    void		fillHist();
    IOPar&		inputHistory()		{ return inpselhist_; }
    IOPar&		steerSelHistory()	{ return steerselhist_; }

    MultiID		attrsetid_;

protected:

    DescSet*		ads_;
    bool		is2d_;
    bool		unsaved_;
    bool		destrondel_;

    IOPar&		inpselhist_;
    IOPar&		steerselhist_;

    void		cleanHist(IOPar&,const DescSet&);

};

} // namespace Attrib
