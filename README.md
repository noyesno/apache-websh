 Websh Tcl - Use Tcl for Web and Others
 ======================================
 
 ![](https://travis-ci.com/noyesno/websh-tcl.svg?branch=master)
 
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

```apache
LoadModule websh_module /path/to/mod_websh.so

<IfModule websh_module>
    WebshConfig /path/to/websh.conf
    AddHandler websh .wsh
</IfModule>
```

### Interpreter Reuse

With mod_websh, Tcl interpreter can be reused between each requests.

This feature can give a big boost to web script response time. 

```tcl
set classid [web::interpclasscfg]

web::interpclasscfg $classid maxrequests 100    ;# handle at most 100 request
web::interpclasscfg $classid maxttl      600    ;# live at most 600 seconds
web::interpclasscfg $classid maxidletime 180    ;# idle at most 180 seconds
```

### Setup and Cleanup

Since the interpreter can be reused, we have the need of setup at the start
once and cleanup at the end.

```tcl
#----------------------------------------------------#
# web::initializer will execute in listed order      #
#----------------------------------------------------#

web::initializer {
  web::logdest add user.-debug file -unbuffered /tmp/test.log
  web::logfilter add *.-debug
  web::log info "initializing interp"
}

#----------------------------------------------------#
# web::finalizer will be executed in reverse order   #
#----------------------------------------------------#

web::finalizer {    
    web::log info "start shutting down interp"
}

web::finalizer {    
    web::log info "just before shutting down interp"
}

#----------------------------------------------------#
# web::command will be dispatched from web::dispatch #
#----------------------------------------------------#

web::command default {
  web::put "hello"
  web::putx /path/to/page.html
}

web::dispatch
```



