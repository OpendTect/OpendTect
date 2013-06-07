#ifndef samplevalueattrib_h
#define samplevalueattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert and Bruno
 Date:          Aug 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Sample Value Attribute

  Shortcut for dummies (like us) to Shift or Mathematics or ... attribute's
  functionality.

*/

namespace Attrib
{

mClass SampleValue : public Provider
{
public:
    static void			initClass();
				SampleValue(Desc&);

    static const char*		attribName()	{ return "SampleValue"; }

protected:

    static Provider*		createInstance(Desc&);

    bool			getInputData(const BinID&,int);
    bool			computeData(const DataHolder&,
	    				    const BinID&,int,int,int) const;

    const DataHolder*		inputdata_;
    int				dataidx_;

};

} // namespace Attrib


#endif

