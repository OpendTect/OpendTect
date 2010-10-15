#ifndef stratlayseqgendesc_h
#define stratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratlayseqgendesc.h,v 1.2 2010-10-15 13:38:41 cvsbert Exp $
________________________________________________________________________


-*/

#include "property.h"
class IOPar;

namespace Strat
{
class RefTree;
class LayerSequence;

/*!\brief Description that can generate layers to add to a sequence.

  genMaterial() needs a float, which must be in range [0,1]: the model position.
  When generating 11 sequences, these will be 0, 0.1, 0.2, ... 0.9, 1.

 */

mClass LayerGenDesc
{
public:	

    virtual const char*	 name() const				= 0;

    virtual const char*	errMsg() const				{ return 0; }
    virtual const char*	warnMsg() const				{ return 0; }

    virtual void	usePar(const IOPar&,const RefTree&)	= 0;
    virtual void	fillPar(IOPar&) const			= 0;

    static LayerGenDesc* get(const IOPar&,const RefTree&);
    mDefineFactoryInClass(LayerGenDesc,factory);

    virtual bool	genMaterial(LayerSequence&,Property::EvalOpts eo
				=Property::EvalOpts()) const	= 0;

};


/*!\brief Collection of LayerGenDesc's that can form a full LayerSequence.  */

mClass LayerSequenceGenDesc : public ObjectSet<LayerGenDesc>
{
public:

			LayerSequenceGenDesc( const RefTree& rt )
			    : rt_(rt)			{}

    bool		getFrom(std::istream&);
    bool		putTo(std::ostream&) const;

    bool		generate(LayerSequence&,float modpos) const;

    const char*		errMsg() const			{ return errmsg_; }
    const BufferStringSet& warnMsgs() const		{ return warnmsgs_; }

    const char*		getUserIdentification(int) const;

protected:

    const RefTree&		rt_;

    mutable BufferString	errmsg_;
    mutable BufferStringSet	warnmsgs_;

};


}; // namespace Strat

#endif
