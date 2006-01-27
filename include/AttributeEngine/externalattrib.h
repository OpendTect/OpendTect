#ifndef externalattrib_h
#define externalattrib_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2004
 RCS:		$Id: externalattrib.h,v 1.1 2006-01-27 13:59:37 cvskris Exp $
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

typedef ExtAttribCalc*(ExtAttribCalcCreator)(const SelSpec&);


class ExtAttribCalcFact
{
public:
    void			add( ExtAttribCalcCreator nc )
				{ creators += nc; }
    ExtAttribCalc*		createCalculator( const SelSpec& spec )
				{
			    	    for ( int idx=0; idx<creators.size(); idx++)
				    {
					ExtAttribCalc* res =
						creators[idx]( spec );
					if ( res )
					    return res;
				    }

				    return 0;
				}

    static ExtAttribCalcFact&	getInstance()
				{
				    static ExtAttribCalcFact inst;
				    return inst;
				}
protected:

    ObjectSet<ExtAttribCalcCreator>	creators;
};

};
#endif
