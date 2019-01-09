 Websh Tcl - Use Tcl for Web and Others
 ======================================
 
 Websh History
 --------------
 
Websh was originally developed by Netcetera AG, Switzerland and was
contributed to the Apache Software Foundation in 2001.

Most recent version is Websh 3.6.0b5 (2009-09-14)

  - Websh:                       http://tcl.apache.org/websh/
  - Apache:                      http://www.apache.org/
  - Netcetera AG, Switzerland:   http://netcetera.ch/
  - Tcl:                         http://tcl.tk/
  
Unfortunately, Apache "discontinued Websh support and development for lack of resources." 

 
Revive Websh Tcl
----------------

Tcl is a good programming language in the sense that it represents program as a list of commands,
which is similar with our nature language. 

Tcl is also powerful with many "fancy" programming concepts available starting from very learly days.

The idea of using Tcl for web programming is natural. Websh is one of such effort.

Websh can be used in one of below 4 ways:

  * Web CGI script
  * Apache mod_websh script
  * Normal Tcl script
  * Websh shell

Although Websh is originally designed for web, I found it's also useful to use it other place. 

The key concept here is "command", just like the one in Tool Command Language (Tcl).


Quick Example
-------------

A hello world Websh script looks like below. `web::command` and `web::dispatch` is the two main commands.

```tcl
web::command hello {
  web::put "hello"
}

web::command world {
  web::put "world"
}

web::dispatch
```


Compiling and installing Websh (Unix)
----------------------------------------

Websh is a pure Tcl extension. The build process is similar with 


```sh
  cd src/unix
  autoconf
  ./configure \
       --with-tclinclude=/usr/include/tcl \
       --with-httpdinclude=/usr/include/apache2
       --enable-threads
  make
  make test
  make apachetest
  make install
```

Make will create three targets: websh3.6.<patch>, which is the standalone 
Websh application (dynamically linked to Tcl and libwebsh3.6.<patch>.so)
and libwebsh3.6.<patch>.so, which is a TEA (Tcl Extension Architecture)
compliant shared object that can be dynamically loaded from within Tcl
using [load libwebsh3.6.<patch>.so]. Both provide the Tcl package
websh. The third target is  mod_websh3.6.<patch>.so (also dynamically
linked to Tcl and libwebsh3.6.<patch>.so), which is the Websh Apache
module.


mod_websh: Websh as Apache Module
------------------------------------

Websh applications can run in either CGI mode or through `mod_websh`.

```
LoadModule websh_module /path/to/mod_websh.so

<IfModule websh_module>
    WebshConfig /path/to/websh.conf
    AddHandler websh .wsh
</IfModule>
```





