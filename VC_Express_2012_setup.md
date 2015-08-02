# Introduction #

This is a short summary of the things which has to be done in your Windows environment so that the VC Express 2012 setup from the repository can be used.


# Details #

Toped uses a number of third party libraries which should be installed in Windows(r) before one attempts a compilation. The trouble is that those libraries can be installed anywhere, yet the VC should be aware of their location. On the other hand the location of those libraries is user dependent and there is no point to restrict the developer how exactly to organize the files in his user account.

So - it should be simple.

  1. Define env variable TPD\_DEV. One way to do this:-
    * In the start menu - type envi and select "Edit Environment Variables for your account"
    * After "Environment Variables" window pops up, click New and define TPD\_DEV variable to point to your development directory which contains all Toped thisrd party dependencies.

  1. Define shortcuts to the external librariesand tools.
    * Run cmd.exe (Windows command prompt)
    * cd into your development directory pointed by the TPD\_DEV env variable defined previously.
    * create links to the third party libraries. For example:
```
mklink /j wxWidgets wxWidgets-2.9.3
mklink /j glew glew-1.9.0
mklink /j zlib zlib-1.2.5
mklink /j GnuWin32 bisonflex
```
    * note that the first argument in the commands above must be repeated verbatim - those are the names used in VC setup.

That should be it. VC should be able now to compile your project.