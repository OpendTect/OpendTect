#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: welltiesetup.h,v 1.7 2009-09-03 09:41:39 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

#include "attribdescid.h"
#include "multiid.h"

#include "wellio.h"
#include <iosfwd>

class IOPar;
class MultiID;

namespace WellTie
{

mClass Setup
{
public:
			Setup()
			    : wellid_(*new MultiID())
			    , attrid_(*new Attrib::DescID())
			    , wvltid_(*new MultiID())
			    , issonic_(true)
			    , isinitdlg_(true)
			    , corrvellognm_("CS Corrected ")
			    {}


			Setup( const Setup& setup ) 
			    : wellid_(setup.wellid_)
			    , attrid_(setup.attrid_)
			    , wvltid_(setup.wvltid_)
			    , issonic_(setup.issonic_)
			    , isinitdlg_(setup.isinitdlg_)
			    , vellognm_(setup.vellognm_)
			    , denlognm_(setup.denlognm_)
			    , corrvellognm_(setup.corrvellognm_)
			    {}	
	
    MultiID			wellid_;
    Attrib::DescID         	attrid_;
    BufferString        	vellognm_;
    BufferString          	denlognm_;
    BufferString          	corrvellognm_;
    MultiID               	wvltid_;
    bool                	issonic_;
    bool 			isinitdlg_;
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static Setup&		defaults();
    static void                 commitDefaults();

protected:
    
};


mClass IO : public Well::IO
{
public:
    				IO(const char* f,bool isrd)
				: Well::IO(f,isrd)
				{}

    static const char*  	sKeyWellTieSetup();
    static const char*  	sExtWellTieSetup();
};



mClass Writer : public IO
{
public:
				Writer(const char* f,const WellTie::Setup& s)
				    : IO(f,false)
				    , wts_(s)
				    {}

    bool          	        putWellTieSetup() const;  
    bool 			putWellTieSetup(std::ostream&) const;

protected:
   
    const WellTie::Setup& 	wts_;
    bool                	wrHdr(std::ostream&,const char*) const;

};


mClass Reader : public IO
{
public:
				Reader(const char* f,WellTie::Setup& s)
				    : IO(f,true)
				    , wts_(s)
				    {}
  
    bool               		getWellTieSetup() const;	
    bool                	getWellTieSetup(std::istream&) const;


protected:
   
    WellTie::Setup& 	wts_;
};

}; //namespace WellTie
#endif
