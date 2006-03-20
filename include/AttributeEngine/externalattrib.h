#ifndef externalattrib_h
#define externalattrib_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: externalattrib.h,v 1.3 2006-03-20 07:44:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BinIDValueSet;
class CubeSampling;
class SeisTrcBuf;


namespace Attrib
{

class SelSpec;
class DataCubes;

/*! Generic class for attribs that does not come from the attribute engine. */


class ExtAttribCalc
{
public:
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
    				/*!<\returns if this object can 
				     compute it or not. */
    virtual const DataCubes*	createAttrib( const CubeSampling&,
				              const DataCubes* ) 	= 0;
    virtual bool		createAttrib( ObjectSet<BinIDValueSet>&) = 0;
    virtual bool		createAttrib(const BinIDValueSet&,
						     SeisTrcBuf&)	= 0;

    virtual bool		isIndexes() const			= 0;
};


struct ExtAttribCalcCreator
{
    virtual ExtAttribCalc*	make(const SelSpec&) const		= 0;
};


class ExtAttribCalcFact
{
public:
    void			add( ExtAttribCalcCreator* nc )
				{ creators += nc; }
    ExtAttribCalc*		createCalculator( const SelSpec& spec )
				{
			    	    for ( int idx=0; idx<creators.size(); idx++)
				    {
					ExtAttribCalc* res =
						creators[idx]->make( spec );
					if ( res )
					    return res;
				    }

				    return 0;
				}

protected:

    ObjectSet<ExtAttribCalcCreator>	creators;
};

ExtAttribCalcFact& ExtAttrFact();

} // namespace Attrib

#endif
