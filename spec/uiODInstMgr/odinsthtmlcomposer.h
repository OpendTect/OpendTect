#ifndef odinsthtmlcomposer_h
#define odinsthtmlcomposer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          May 2012
 RCS:           $Id: odinsthtmlcomposer.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "objectset.h"

namespace ODInst
{

class PkgProps;
class PkgSelMgr;
class PkgGroupSet;

class HtmlComposer
{
public:
			HtmlComposer();
			~HtmlComposer();

    BufferString	generateHTML(const PkgProps&,PkgSelMgr* psl=0);
    BufferString	generateMainSelHtml(ObjectSet<const ODInst::PkgProps>&,
					    PkgSelMgr*,const PkgGroupSet&);
protected:

    void		generateActionHtml(const PkgProps&,
					    BufferString&);
    void		generateActionHtml( const ODInst::PkgProps& pp,
					 BufferString& html, PkgSelMgr* ) const;
    
    void		generatePackageTableCell(const ODInst::PkgProps&,
					    BufferString&,BufferString&,
					    BufferString&, PkgSelMgr*,
					    const PkgGroupSet&) const;

    BufferString	html_;
};

} //namespace ODInst


static const BufferString htmlprot = "http://";
static BufferString odprotocol = "od://";
static const char* installcmd = "install";
static const char* uninstallcmd = "uninstall";
static const char* detailscmd = "details";
static const char* closedetailcmd = "closedetail";
static const char* filelistcmd = "filelist";

#endif
