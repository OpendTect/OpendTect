#ifndef stratlayseqattribcalc_h
#define stratlayseqattribcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2011
 RCS:		$Id: stratlayseqattribcalc.h,v 1.1 2011-01-13 14:52:13 cvsbert Exp $
________________________________________________________________________

-*/


namespace Strat
{
class LayerModel;
class LaySeqAttrib;
class LayerSequence;


/*!\brief calculates attributes from layer sequences */

mClass LaySeqAttribCalc
{
public:

    			LaySeqAttribCalc(const LaySeqAttrib&,const LayerModel&);

    float		getValue(const LayerSequence&) const;

protected:


};

}; // namespace Strat

#endif
