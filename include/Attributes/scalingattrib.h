#ifndef scalingattrib_h
#define scalingattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          December 2004
 RCS:           $Id: scalingattrib.h,v 1.4 2005-08-05 10:51:52 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"
#include "basictask.h"
#include "runstat.h"

/*!\brief Scaling Attribute

  Scaling gate=

  Calculates the squared sum of the gate's samples divided by the number of
  samples in the gate.

Input:
0               Data

Outputs:
0               The scaled trace
*/
    

namespace Attrib
{

class Scaling: public Provider
{
public:
    static void		initClass();
			Scaling(Desc&);

    static const char*  attribName()		{ return "Scaling"; }
    static const char*  scalingTypeStr()	{ return "scalingtype"; }
    static const char*  powervalStr()		{ return "powerval"; }
    static const char*  gateStr()		{ return "timegate"; }
    static const char*  factorStr()		{ return "scalefactor"; }
    static const char*  statsTypeStr()		{ return "statstype"; }
    static const char*  statsTypeNamesStr(int type);
    static const char*  scalingTypeNamesStr(int type);

protected:

    static Provider*    createInstance(Desc&);
    static Provider*    internalCreate(Desc&,ObjectSet<Provider>& existing);
    static void         updateDesc( Desc& );

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    void                checkTimeGates( const TypeSet<Interval<float> >& oldtgs,
				      TypeSet< Interval<int> >& newsampgates,
				      int t0, int nrsamples) const;
    void                scaleTimeN(const DataHolder&, int, int) const;


    int				scalingtype;
    int				statstype;
    float                       powerval;
    TypeSet< Interval<float> >	gates;
    TypeSet<float>              factors;
    const DataHolder*		inputdata;
};

}; // namespace Attrib


/*!\mainpage Standard Attributes

  This module contains the definition of the 'standard' OpendTect attributes.
  Contained are attributes like Energy, Similarity, Volume Statistics, etc.
  The base class for all attributes is the AttribCalc class.

  The Attribute factories are defined in the Attribute Engine module
  (AttribEng).

  If you want to make your own attributes, please consult the Programmer's
  manual, section 'Plugins'. You'll find an annotated example of the Coherency
  attribute implementation.

*/


#endif
