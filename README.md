# Pwee PHP Extension

Pwee is a PHP extension that gives web developers the ability to expand the PHP runtime
environment. Developers can use XML to define and add custom constants and variables that
are accessible to each script. The lifetime and scope of constants and variables can be
controlled individually and the scope of variables can be expanded so they are persistent
across multiple requests to the same script executor.


Requirements
---

* [PHP 4.1 or later](http://www.php.net/)

Certain features are not available under Windows.


Installation
---

Download and extract the Pwee tarball to the PHP extension directory. PHP extensions are
located in the /ext subdirectory of the PHP installation. These instructions assume you
installed PHP in `/usr/local/src`.

### Static Linking

For the best performance impact it makes sense to link Pwee directly into PHP.

	% tar xvfz pwee.tar.gz -C /usr/local/src/php/ext
	% cd /usr/local/src/php
	% ./buildconf
	% ./configure --with-pwee [other options]

### Shared Library (DSO)

	% tar xvfz pwee.tar.gz -C /usr/local/src/pwee
	% cd /usr/local/src/pwee
	% phpize
	% ./configure --with-pwee=shared [other options]
	% make
	% make install

### Shared Library

	% tar xvfz pwee.tar.gz -C /usr/local/src/php/ext
	% cd /usr/local/src/php
	% ./buildconf
	% ./configure --with-pwee=shared [other options]

## Configuration

After you install the package and build PHP all that's left is to create an XML environment
definition and add `pwee.sysconfpath` or `pwee.userconfpath` settings to your php.ini.

### Virtual Hosts

If you want to define the environment based on the VirtualHost you can modify your
httpd.conf and add a php_value option for the directory that is the DocumentRoot
for the VirtualHost.

	<VirtualHost _default_:80>
	  ServerName www.domain.com
	  DocumentRoot /var/www/www.domain.com
	  <Directory /var/www/www.domain.com>
		php_value pwee.userconfpath "/var/www/www.domain.com/pwee.conf.xml"
	  </Directory>
	</VirtualHost>

If you want to define the environment based on a particular directory you can add a php_value
option to a .htaccess file. In order for this to work the AllowOverride property for the
<Directory> that contains the .htaccess must include the value Options or All.

	php_value pwee.userconfpath "/var/www/www.domain.com/pwee.conf.xml"

It is more efficient to define the Pwee configuration in the VirtualHost definition
in httpd.conf than using .htaccess. Either one is suitable and Pwee will cache the
environment definition internally for efficiency.


php.ini Settings
---

There are a number of different settings that control how Pwee works.

* `pwee.sysconfpath` - Path to a system wide XML environment definition file.
  This value can only be set in php.ini. There is no default value.
* `pwee.userconfpath` - Path to a user defined XML environment definition file.
  This value should be set on a per directory basis in either httpd.conf or .htaccess.
  There is no default value. Once a user environment is loaded it is cached for
  efficiency. If you modify the XML definition and want Pwee to reload it you need
  to append a serial number to the end of the path. The serial number is specified
  following a colon. For example conf.xml:1 would specify serial number 1. Any
  change in this part of the path will cause Pwee to reload the definition.
  Only one userconfpath is active at any given time.
* `pwee.userconf_allow` - Determines if Pwee allows user environments to be defined
  using pwee.userconfpath. The default is Yes. This value can only be set in php.ini.
* `pwee.register_net_constants` - Determines if Pwee registers constants for network
  information such as server IP addresses. The default is Yes.
* `pwee.net_constant_prefix` - Prefix to use for network constants. Only applicable
  when pwee.register_net_constants is Yes. The default prefix is 'SERVER'.
* `pwee.register_uid_constants` - Determines if Pwee registers constants that uniquely
  identify the executor and request. The default is Yes.
* `pwee.exposeinfo` - Determines if constants and variables are displayed by phpinfo().
  The default is Yes.


Default Environment Constants
---

Besides the constants and variables defined by XML environment definitions,
Pwee defines some constants itself.

* `EXECUTOR_UID` - Each script executor is assigned a unique identifier.
  The identifier is 36 characters long and has the format c5e57bc9-d2a4-4521-a29b-ff89382ae878.
  The lifetime of an executor depends on how you invoke PHP. For example, if you
  are using Apache with mod_php each Apache process is an executor.
* `EXECUTOR_REQUEST_UID` - Each script execution (request) is assigned a unique identifier.
  The identifier is 41 characters long and has the format 5017d980-f2f1-4b08-a1b8-7e33d161f346-0e33.
* `SERVER_IFADDR_LO` - Defines the IP address of the loopback network device.
  Almost always 127.0.0.1.
* `SERVER_IFADDR_ETH[0...]` - Defines the IP address of other network devices.
  One constant will be created for each device. There is almost always a
  SERVER_IFADDR_ETH0 constant.
* `SERVER_IFADDR` - Defines the IP address of the first network device that is
  not the loopback device. Almost always the same as SERVER_IFADDR_ETH0.
* `SERVER_HOSTNAME` - Defines the server hostname. The actual value depends on
  your server network configuration. Under Linux you can see the hostname for
  your server using the -f option to the <i>hostname</i> command.
* `SERVER_HOSTSHORTNAME` - Defines the server short hostname. The actual value
  depends on your server network configuration.  Under Linux you can see the
  short hostname for your server using the -s option to the <i>hostname</i> command.
* `SERVER_HOSTDOMAIN` - Defines the server domain name. The actual value depends
  on your server network configuration.  Under Linux you can see the short hostname
  for your server using the -d option to the <i>hostname</i> command.


Exported Functions
---

* `pwee_info()` - Prints out the same information as phpinfo() except it only
  displays Pwee module information.


XML Environment Definition
---

### Document Type Definition

	<!DOCTYPE Environments
	[
	  <!ELEMENT Environments ((Package|Application)+)>
	  <!ELEMENT Application ((Server|Constants|Variables)+)>
		<!ATTLIST Application name CDATA #REQUIRED>
		<!ATTLIST Application namespace CDATA #REQUIRED>
		<!ATTLIST Application comment CDATA #IMPLIED>
	  <!ELEMENT Package ((Server|Constants|Variables)+)>
		<!ATTLIST Package name CDATA #REQUIRED>
		<!ATTLIST Package namespace CDATA #REQUIRED>
		<!ATTLIST Package comment CDATA #IMPLIED>
	  <!ELEMENT Server ((Constants|Variables)+)>
		<!ATTLIST Server ip CDATA #IMPLIED>
		<!ATTLIST Server interface CDATA #IMPLIED>
		<!ATTLIST Server hostname CDATA #IMPLIED>
		<!ATTLIST Server domain CDATA #IMPLIED>
		<!ATTLIST Server comment CDATA #IMPLIED>
	  <!ELEMENT Constants (Constant+)>
		<!ATTLIST Constants prefix CDATA #IMPLIED>
		<!ATTLIST Constants comment CDATA #IMPLIED>
	  <!ELEMENT Constant EMPTY>
		<!ATTLIST Constant name CDATA #REQUIRED>
		<!ATTLIST Constant value CDATA #REQUIRED>
		<!ATTLIST Constant type (string|long|boolean|double) "string">
		<!ATTLIST Constant comment CDATA #IMPLIED>
	  <!ELEMENT Variables (Variable+)>
		<!ATTLIST Variables prefix CDATA #IMPLIED>
		<!ATTLIST Variables scope (request|executor) "request">
		<!ATTLIST Variables comment CDATA #IMPLIED>
	  <!ELEMENT Variable EMPTY>
		<!ATTLIST Variable name CDATA #REQUIRED>
		<!ATTLIST Variable value CDATA #REQUIRED>
		<!ATTLIST Variable type (string|long|boolean|double) "string">
		<!ATTLIST Variable scope (request|executor) #IMPLIED>
		<!ATTLIST Variable comment CDATA #IMPLIED>
	]>

### XML Elements

#### Environments

The Environment element is the document root. It must contain one or more
Application or Package elements.

#### Application and Package

Constants and variables are grouped by Application or Package elements.
Currently there is no difference between these two elements as they have the same
attributes. Application and Package elements must contain one or more Server,
Constants or Variables elements.

* `name` - Each Application and Package must be named. This is a string value
  and is used for informational purposes only.
* `namespace` - Each Application and Package must define a namespace. The namespace
  is a string that will prefix the name of each constant and variable in the
  Application or Package. The namespace value will be separated from the constant
  or variable name with an underscore character.
* `comment` - The element can have an associated string comment for documentation purposes.

#### Server

The Server element is used to control the constants and variables that are defined
for particular servers. You can restrict constants and variables based on the server
IP address, hostname or domain. Server elements must contain one or more Constants
and Variables sections. These elements will only apply when the conditions of the Server
element are met.

* `ip` - You can restrict the environment defined for a particular server by specifying the
  exact IP address of the server. If you want an environment to be applied to multiple servers
  you can set the IP address to a particular subnet. For example, if you want to set the
  environment for only the server 192.168.1.10 you would set "ip = 192.168.1.10".
  If you have more than one server and want to set the environment for all servers in
  a particular Class C subnet (192.168.1.*) you would set "ip = 192.168.1." (note
  the trailing period).
* `interface` - When defining the environment based on IP address the default is to use the
  IP address of the eth0 network interface. If you want to use the IP of a different
  network interface you can use this attribute. This attribute is only relative if the
  ip attribute is set.
* `hostname` - You can define the environment based on the server hostname instead
  of the IP address by setting a value for this attribute instead of the ip attribute.
  The environment will only be applied if there is an exact match between this value
  and the short hostname of the server. Under Linux you can see the short hostname
  for your server using the -s option with the <i>hostname</i> command. If the ip
  attribute is set the hostname attribute is ignored.
* `domain` - You can define the environment based on the server domain instead of
  the IP address by setting a value for this attribute instead of the ip attribute.
  The environment will only be applied if there is an exact match between this value
  and the hostname domain of the server. Under Linux you can see the hostname domain
  for your server using the -d option with the <i>hostname</i> command. If the ip
  or hostname attribute is set the domain attribute is ignored.
* `comment` - The element can have an associated string comment for documentation purposes.

#### Constants

Constant elements are grouped together by a Constants element. Constants elements
must contain one or more Constant elements.

* `prefix` - Prefix is a string value that will prefix the name of each constant
  in the element. The prefix will be separated from the constant name with an underscore character.
This prefix is applied in addition to the namespace prefix for the Application or Package.
* `comment` - The element can have an associated string comment for documentation purposes.

#### Variables

Variable elements are grouped together by a Variables element. Variables elements
must contain one or more Variable elements

* `prefix` - Prefix is a string value that will prefix the name of each variable
  in the element. The prefix will be separated from the variable name with an
  underscore character. This prefix is applied in addition to the namespace prefix
  for the Application or Package.
* `scope` - You can apply a scope to a group of Variable elements by setting the
  scope of the Variables element. See the description of the Variable scope attribute
  for a list of string values that can be used. The default scope is request.
* `comment` - The element can have an associated string comment for documentation purposes.

#### Constant

Constant elements allow you to define constant values cannot be changed during
script execution.

* `name` - Each constant must be named. The actual name of the constant accessible
  to scripts depends on the namespace of the Application or Package and whether the
  Constants element defined a prefix.
* `value` - The value for the constant. The value must be specified as a string but
  is converted to a PHP type based on the type attribute.
* `type` - The constant type can be string, long, boolean or double. If the type
  is not specified it is assumed to be a string.
* `comment` - The element can have an associated string comment for documentation purposes.

#### Variable

Variable elements allow you to define variable values that can be changed during
script execution. Depending on the scope of the variable the lifetime of the
variable can be changed.

* `name` - Each variable must be named. The actual name of the variable accessible
  to scripts depends on the namespace of the Application or Package and whether the
  Variables element defined a prefix.
* `value` - The value for the variable. The value must be specified as a string but
  is converted to a PHP type based on the type attribute.
* `type` - The variable type can be string, long, boolean or double. If the type
  is not specified it is assumed to be a string.
* `scope` - Scope controls the lifetime of variable. The scope can be request or executor.
  If the variable has request scope the value of the variable will be reset for
  each request. This is the default scope. The more interesting scope is executor.
  This scope allows the variable to retain its value across multiple requests.
  This behavior is greatly dependent on how PHP is invoked. For example,
  if you use Apache with mod_php each individual Apache process is considered an
  executor. Requests handled by each executor will be able to access and modify the
  value available to the next request. Executor scope does not (yet) allow you to define
  "application" variables that are shared between all executors.
  If you dynamically load the extension as a shared library the executor scope becomes
  request scope because the lifetime of the executor and request are the same.
  If a Variable scope is set it overrides the scope of the Variables element.
* `comment` - The element can have an associated string comment for documentation purposes.


Example
---

An example will help illustrate what kind of problem Pwee solves. If your web application uses
a database, each script needs to know the hostname of the database server (as well as the username
and password). This is easily achieved by hardcoding the hostname in your script.

	$db = new Database;
	$db->hostname = "foo";
	$db->user = "user";
	$db->password = "password";

This is very simplistic however. If you have both a development and production network you need to
add logic to the script so your application uses the appropriate database based on where it's running.
One way to achieve this is to base it on the IP address of the server.

	$db = new Database;
	if (ereg("^192.168.1.", _SERVER["SERVER_ADDR"]))
	  $db->hostname = "foo";
	else
	  $db->hostname = "bar";

This logic fails though if you run your script from the shell because SERVER_ADDR is not defined.
If you run scripts from the shell you have to work up some other way to determine weather you should be using the
production or development database server. The fact remains however that every single time a script
is run it has to examine its environment in order to determine what it should do.

If you want to make the process more efficient an alternative is to create server specific include
files that define the environment without having to examine anything. In this case you would create
two scripts, say production.config.inc and development.config.inc. Each script would
define the constant you can use when connecting to the database.

config.inc links to <i>production.config.inc</i> on <i>production</i> servers:

	define(DATABASE_HOSTNAME, "foo");

config.inc links to <i>development.config.inc</i> on <i>development</i> servers:

	define(DATABASE_HOSTNAME, "bar");

At the top of each script you just include the configuration script.

	require_once("config.inc");

	$db = new Database;
	$db->hostname = DATABASE_HOSTNAME;

This solution works well but is very error prone because you have to manage two files
that need to have the same name but contain different content. Bad things can happen if
you accidentally overwrite the file on the wrong server. If you have more than two networks it becomes
even more error prone.

Using the Pwee extension you would just create an environment definition in XML.

	<?xml version="1.0"?>
	<Environments>
	  <Application name="My web application" namespace="">
		<Constants>
		  <Constant name="DATABASE_USER" value="user" />
		  <Constant name="DATABASE_PASSWORD" value="password" />
		</Constants>
		<Server ip="192.168.1.">
		  <Constants>
			<Constant name="DATABASE_HOSTNAME" value="bar" />
		  </Constants>
		</Server>
		<Server ip="10.10.0.">
		  <Constants>
			<Constant name="DATABASE_HOSTNAME" value="foo" />
		  </Constants>
		</Server>
	  </Application>
	</Environments>

This environment defines USER and PASSWORD as being server independent whereas HOSTNAME
depends on the IP address of the server. Depending on how you invoke PHP, the environment
definition will only be read once and the appropriate constants will be added to the runtime
environment for all your scripts. The scripts don't need to have any extra logic or waste
any time determining what values for which constants to use.

This example is a good illustration for another problem common to PHP web applications.
Almost every script contains include and require statements. Take the simple script above.

	require_once("config.inc");

The question is where exactly is config.inc? If PHP doesn't find this script in the local
directory it's going to search your include path. What if you have multiple projects and each
has its own config.inc? You could add more path information.

	require_once("projectA/config.inc");

But you still might be forcing PHP to search the include path. In order to streamline performance
and keep PHP from searching for the file on every script execution you need to specify the full
path to the file. (Include paths also become an issue if you have a lot of VirtualHosts. You don't want to
include VirtualHost specific directories in the main include_path. If you set the include_path
in .htaccess files you run into trouble when you execute PHP from the shell.)

	require_once("/full/path/to/projectA/config.inc");

This works but becomes a nuisance if the full path to your project changes. A common solution
to avoid full paths is to include one main configuration file that defines a constant or variable
that can be used in subsequent include statements.

	require_once("projectA/config.inc");
	- or -
	require_once("/full/path/to/projectA/config.inc");

	// config.inc set $INCLUDE_DIR

	require_once("$INCLUDE_DIR/classA.php");
	require_once("$INCLUDE_DIR/classB.php");

This is a good solution but you are still forced to either search the include path or
have full paths in your scripts. Pwee provides a better solution. You can define a Pwee
environment that defines INCLUDE_DIR as either a variable or as a constant.

	<?xml version="1.0"?>
	<Environments>
	  <Application name="www.domain.com" namespace="">
		<Variables>
		  <Variable name="INCLUDE_DIR" value="/full/path/to/projectA" />
		</Variables>
	  </Application>
	</Environments>

	<?xml version="1.0"?>
	<Environments>
	  <Application name="www.domain.com" namespace="">
		<Constants>
		  <Constant name="INCLUDE_DIR" value="/full/path/to/projectA/" />
		</Constants>
	  </Application>
	</Environments>

Then use that variable or constant as before.

	require_once("$INCLUDE_DIR/classA.php");
	- or -
	require_once(INCLUDE_DIR . "classA.php");


Why
---

Q) Why not just include a file at the top of each script that defines all the
   constants and variables you need?

The short answer - efficiency. The example above provides a good illustration. You are
forced to either make PHP search the include path to find the include file
or you must have at least one full path hard coded in all your scripts.
Depending on your situation and performance requirements and what the include file
actually needs to do you may be better off using an include file. If your include
file just defines a bunch of constants and global variables you're better off with
Pwee. If the script you include contains application logic you must use an include
file. Pwee also lets you benefit from executor variables which an include file cannot
provide. Even when using include files Pwee can be useful because it will define
constants (such as the server's IP address) that you can use to make server specific
decisions. If you use a caching product like Zend Accelerator, APC, PHPA, etc.
the performance hit for including a file is probably negligible but Pwee may be
able to reduce it to zero.

Q) Why not just use the php.ini setting auto_prepend_file?

First read the answer for the question above. Using auto_prepend_file is better
than the include file approach because instead of hard coding a path in all your
scripts you just hard code a script path in your php.ini. The downside still is
that you are wasting time defining constants and variables with each script execution
that could be done once per executor. As with the include file approach in general
using auto_prepend_file has its place too.


Sample Packages
---

### Smarty Template Engine

If you use the [Smarty Template Engine](http://www.smarty.net/)
here is a Pwee package that defines constants that Smarty will use. Smarty uses the
SMARTY_DIR constant to find the location of the Smarty classes. The other constants are useful
only if you change the hardcoded paths in the Smarty class definition.

	<Package name="Smarty" namespace="SMARTY">
	  <Constants>
		<Constant name="DIR" value="/var/www/domain.com/classes/Smarty/" />
		<Constant name="TEMPLATE_DIR" value="/var/www/domain.com/templates/" />
		<Constant name="COMPILE_DIR" value="/var/www/domain.com/cache/Smarty/templates_c/" />
		<Constant name="CACHE_DIR" value="/var/www/domain.com/cache/Smarty/cache/" />
		<Constant name="CONFIG_DIR" value="/var/www/domain.com/classes/Smarty/configs/" />
	  </Constants>
	</Package>

### ADOdb Database Library for PHP

If you use the [ADOdb Database Library for PHP](http://adodb.sourceforge.net/) here
is a Pwee package that defines both constants and variables that ADOdb can use. ADOdb uses the
ADODB_DIR constant to find the location of the ADOdb classes. ADOdb will use the ADODB_CACHE_DIR
variable to find the location of the cache directory. If you are using ADOdb for session
management you can define variables the session handler will use. You can include these variables
in a Server block so you can have different connection parameters for production and development
networks.

	<Package name="ADODB" namespace="ADODB">
	  <Constants>
		<Constant name="DIR" value="/var/www/domain.com/classes/adodb" />
	  </Constants>
	  <Variables>
		<Variable name="CACHE_DIR" value="/var/www/domain.com/cache/adodb" />
	  </Variables>
	  <Server ip="192.168.0." comment="development servers">
		<Variables prefix="SESSION">
		  <Variable name="DRIVER" value="mysql" />
		  <Variable name="USER" value="user" />
		  <Variable name="PWD" value="password" />
		  <Variable name="DB" value="database" />
		  <Variable name="CONNECT" value="server" />
		  <Variable name="TBL" value="sessions" />
		</Variables>
	  </Server>
	</Package>
