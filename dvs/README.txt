
				-----
				 DVS
				-----

DVS is a set of simple wrapper scripts around cvs. It is basically a
cvs front-end, dedicated for use with OpendTect and the dGB
environment.

========================================================================
Setting up dvs
========================================================================

At dGB, dvs is installed in the directory "/users/appman/dvs". Just
setenv OD_I_AM_A_DEVELOPER and source the script
"/users/appman/dvs/set_dvs_env.csh" from your .login and you should be
fine.

For external users, dvs must be checked out from the cvs repository.
The simplest way to do this is by using read-only cvs access:

    setenv CVSROOT :pserver:cvsanon@cvs.opendtect.org:/cvsroot
    cvs login
    cvs co dvs

Setting up dvs from csh:

    dvs needs some invironment variables to be set, in order to
    function properly. The most important one is DGB_CVS_DIR. This
    needs to point to the dvs directory. If you have dvs in your $WORK
    directory, you don't have to set DGB_CVS_DIR. Otherwise, set it in
    your .login.

    The set_dvs_env.csh in the dvs directory, takes care of setting
    dvs up, from within csh. Just set either $WORK or $DGB_CVS_DIR in
    your .login file, and then "source <dvs dir>/set_dvs_env.csh"


Setting up dvs with read/write access from csh:

    just add a "setenv OD_I_AM_A_DEVELOPER yes" to your .login, before
    sourcing set_dvs_env.csh.


========================================================================
Setting up read/write ssh acces to the cvs server (cvs.opendtect.org)
========================================================================

1) make sure you have a ssh key -- in ~/.ssh there should be a id_dsa
and a id_dsa.pub file.

If not: ssh-keygen -tdsa

for example:

    18/users/helene-> ssh-keygen -tdsa
    Generating public/private dsa key pair.
    Enter file in which to save the key (/users/helene/.ssh/id_dsa): 
    Enter passphrase (empty for no passphrase): 
    Enter same passphrase again: 
    Your identification has been saved in /users/helene/.ssh/id_dsa.
    Your public key has been saved in /users/helene/.ssh/id_dsa.pub.
    The key fingerprint is:
    bf:de:3b:11:44:7e:0e:0a:82:6d:21:82:3e:72:bb:0f helene@dgb18

just use an empty passphrase -- otherwise you still have to type
a password...

2) Mail your id_dsa.pub file to arend (at) opendtect (dot) org

   <<< ONLY the .pub file!! >>>

   The id_dsa key-file should remain secret. Please realise that the
   id_dsa file is a security risk. With that key, one can get shell
   access to our webserver. So, please take all measures at your
   disposal to make sure noone but yourself has access to your id_dsa
   key.


3) login to the opendtect cvs server as cvs<username>:

    ssh "cvs$USER"@cvs.opendtect.org

for example:

    18/users/helene-> ssh "cvs$USER"@cvs.opendtect.org
    The authenticity of host 'cvs.opendtect.org (80.85.129.46)' can't be established.
    RSA key fingerprint is 62:33:b9:ba:ff:dc:81:8d:6e:8c:25:02:7a:d9:28:f5.
    Are you sure you want to continue connecting (yes/no)? yes
    Warning: Permanently added 'cvs.opendtect.org,80.85.129.46' (RSA) to the list of known hosts.
    cvshelene@cvs.opendtect.org's password: 
    [cvshelene@dgbserver cvshelene]$ 

Just type "yes" to accept the host key to be added to your
known_hosts. That should log you in to the server.

Now verify that if you log-out and re-login, that you can
automatically login without typing any password.

========================================================================
Checking out a new work directory
========================================================================

Once you have access to the cvs server, you can check-out a new work
directory. There is a "mk_work_dir" script in the dvs directory, that
should take care of setting up your work directory. 

In case you want read/write cvs access in your work directory, make
sure you have setup ssh and have set OD_I_AM_A_DEVELOPER in your
.login.

Then execute the mk_work_dir script. It accepts two arguments. The
first is the work-directory you want to prepare. The second argument
is optional and can only be used within dgb, because the internal cvs
server is not reachable from the outside.


