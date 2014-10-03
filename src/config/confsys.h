/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef SRC_CONFIG_CONFSYS_H_
#define SRC_CONFIG_CONFSYS_H_

/**
 * @file	confsys.h

1) Confsys Architecture

Confsys is designed as server/client model with IPC.  The IPC is TCP
over UNIX domain socket.  OVSDB, OF-Config, lagosh are confsys client.
confsys server is implemented as a library.  In Lagopus configuration
the confsys server is linked as a part of agent process.  So the
confsys server is allocated during agent initialization.

These client uses UNIX domain socket to send and receive confsys
message.  This is very similar to popular TCP based IPC such as HTTP.
The callback function is executed inside confsys server.  So confsys
client such as OVSDB, OF-Config, lagosh are no need to worry about
callback function.  The confsys client only need to open IPC socket
then send and receive message using the socket.

<pre>
               ---
+-----------+   |
|   OVSDB   +---|
+-----------+   |
+-----------+   |    +-------------+ callback()
| OF-Config +---|----| Confsys(DB) |<---------->agent,dpmgr
+-----------+   |    +-------------+
+-----------+   |
|  lagosh   +---|
+-----------+   |
               ---
          UNIX domain socket
</pre>

Confsys server has database inside.  There are two types of database.
One is schema DB.  Another one is configuration DB.  In schema DB
there are three different category tree.  "exec" tree, "show" tree,
"schema" tree.  All of these trees are implemented by "struct cnode"
which is generic tree implementation.

<pre>
+----------------------------------------------+
| +--"show"   For "get" operation.             |
| +--"exec"   For "exec" operation server side |
| +--"schema" For "set/unset" operation.       |
+----------------------------------------------+
Schema DB
</pre>

<pre>
+------------+
| +--"config"|
+------------+
Config DB
</pre>

When server recieves CONFSYS_MSG_TYPE_GET message, it parse "show"
tree of Schema DB.  When there is a match and the matched node has
callback function, confsys server execute the callback then return
message to the caller.

When server recieves CONFSYS_MSG_TYPE_{SET,UNSET}, it parse "schema"
tree of the Schema DB.  It also checks Config DB tree so that the
configuration already exists or not.

When server recieves CONFSYS_MSG_TYPE_EXEC message, it parse "exec"
tree of the Schema DB.  This is necessary to process "server side
command" such as "stop-process".

To access these DB and tree.  There are pointer inside "struct
cserver".  We are expecting to allocate "struct cserver" in agent
thread.

<pre>
struct cserver
{
  ...
  struct cnode *show;

  struct cnode *exec;

  struct cnode *schema;

  struct cnode *config;
  ...
};
</pre>

So all of above DB and tree can be accessible from cserver->{show,
exec, schema, config}.  When developer need to take a look into detail
of the DB, here is the starting point for it.

2) XML schema file.

When agent start up, it allocate "struct cserver".  In allocating
"struct cserver" process, it loads XML schema file. There are two
schema file.

o operational.xml
o configuration.xml

The file location is ${sysconfdir}/lagopus/.  Default location is
/usr/local/etc/lagopus/.

The operational.xml file defines both lagosh operational command and
Schema DB's "show" tree.  Here is snippiest of operational.xml

<pre>
operational.xml
===============
<cnode:schema>
  <!-- Mode: Operational -->
  <cnode name="configure"
	 help="Manipulate software configuration information"/>
  </cnode>
  ...
  <!-- Show commands -->
  <cnode name="show" help="Show information about the system" exec="true">
  </cnode>
</cnode:schema>
</pre>

The top node is treated as lagosh operational command.  The <cnode
name="show"/> is treated as top node of "show" tree in the Schema DB.
When we need to add "show" node for CONFSYS_MSG_TYPE_GET,
operational.xml's show tree is the place to add.

The another XML schema configuration.xml is for both "exec" and
"schema" tree in Schema DB.

<pre>
configuration.xml
=================
<cnode:schema>
  <!-- Exec -->
  <cnode name="exit"
	 help="Exit from this configuration level"/>
  <cnode name="show"
	 help="Show the configuration (default values may be suppressed)"/>
  <cnode name="save"
	 help="Save the configuration"/>
  <cnode name="stop-process"
	 help="Stop lagopus process"/>

  <!-- Configuration -->
  <cnode name="set"
	 help="Set value of a parameter or create a new element">
  </cnode>
</cnode:schema>
</pre>

The top node such as "exit", "show" is treated as "exec" tree in the
Schema DB.  The <cnode name="set"/> is treated as a top node of
"schema" tree in the Schema DB.  So when new "schema" node to be
added, it should be defined under "set" node.  Even if it is under
"set" it is used for both CONFSYS_MSG_TYPE_SET and
CONFSYS_MSG_TYPE_UNSET.

In summary:

<pre>
o operational.xml   +- Top node is lagosh operational command
                    +-"show" node is Schema DB "show"
o configuration.xml +- Top node is Schema DB "exec"
                    +-"set" node is Schema DB "schema"
</pre>

The reason of this complexity is come from the compatibility with
Juniper CLI, JUNOS XML and JUNOScript.  Juniper CLI has two mode one
is operational mode one is configuration mode.  So to fit to CLI
definition, this categorization is the most efficient.

3) Defining schema and callback().

To define the schema and callback function, we need to understand
basic command path notation.  Basic idea is taken from XML and XPath.
Let's assume we are going to define controller set function for
specific bridge.  The command path is same as XPath with space
delimiter.  The reason of why we do not use '/' or '.' as delimiter is
because both '/' and '.' is used as part of interface name. '/' is
used for sub interface name. '.' is used for vlan tag interface name.

When we define command path like this:

bridge-domain br0 controller 10.0.0.1

The schema looks like this.

<pre>
<!-- Bridge domains -->
<cnode name="bridge-domains" multi="true" help="Bridge domains">
  <cnode name="WORD" help="Bridge name" leaf="yes">
    <cnode name="controller" help="OpenFlow controller" multi="true">
      <cnode name="A.B.C.D" help="Controller address" leaf="yes">
      </cnode>
    </cnode>
  </cnode>
</cnode>
</pre>

When name starts from lower letter it is treated as keyword.  The
command must have exact match to the keyword.  When name starts from
upper letter it is variable.  To the variable match, following name is
pre-defined.

<pre>
-------------------------------------
WORD       - Matches word
A.B.C.D    - Matches IPv4 address
A.B.C.D/M  - Matches IPv4 prefix
X:X::X:X   - Matches IPv6 address
X:X::X:X/M - Matches IPv6 prefix
MIN-MAX    - Matches unsigned 64 integer range e.g. 1-65535
-------------------------------------
</pre>

With this definition the input:

bridge-domain br0 controller 10.0.0.1

will match to:

bridge-domain WORD controller A.B.C.D

Adding to the variable match, there are several attribute is defined
in above schema.

<pre>
cnode attributes
-------------------------------------
"name"  - Name of the node.  lower letter is keyword upper letter is vaiable.
"multi" - Child node can have more than one node.
"leaf"  - Even if the node has child node, it is treated as leaf node.
"value" - Child node is treated as value node.
-------------------------------------
</pre>

Internally, all of configuration is structured by "sturct cnode" only.
We can configure specific node can be "leaf", "multi" and "value"
node.

In above schema, "WORD" has attribute "leaf" so:

bridge-domain br0

is also valid command.  When node does not have attribute "leaf" and
the node has child node, it parsed as incomplete command.

More precisely

<pre>
<cnode name="a">
  <cnode name="b">
    <cnode name="c">
    </cnode>
  </cnode>
</cnode>
</pre>

In this case, set/unset operation to

a b c

is valid. But set/unset operation to

a b

is invalid.  Because, "b" has child node "c" so it is not a leaf node.
To make set/unset operation to "b" node, set leaf="yes" to "b".

<pre>
<cnode name="a">
  <cnode name="b" leaf="yes">
    <cnode name="c">
    </cnode>
  </cnode>
</cnode>
</pre>

This schema make it possible to set/unset to

a b

By default, each node can have only one corresponding config node.  So

<pre>
<cnode name="a">
  <cnode name="WORD">
  <cnode>
</cnode>
</pre>

This can have

a node1

As configuration display,this is:

a {
  node1;
}

When we set

a node2

By default we can't add this node because node1 already exists.  To
enable multiple config node for one schema node, multi="yes" can be
specified.

<pre>
<cnode name="a" multi="yes">
  <cnode name="WORD">
  <cnode>
</cnode>
</pre>

With this schema, we can set multiple child node for one schema node.
So we can configure:

a node1
a node2

at the same time.  This create configuration like this:

a {
  node1;
  node2;
}

When node has "value" attribute, it shows like "value" node:

<pre>
<cnode name="a">
  <cnode name="b">
    <cnode name="c">
      <cnode name="d">
        <cnode name="val"/>
      </cnode>
    </cnode>
  </cnode>
</cnode>
</pre>

This defines configuration looks like:

<pre>
a {
  b {
    c {
      d {
        val;
      }
    }
  }
}
</pre>

When we add "value" attribute:

<pre>
<cnode name="a">
  <cnode name="b">
    <cnode name="c">
      <cnode name="d" value="yes">
        <cnode name="val"/>
      </cnode>
    </cnode>
  </cnode>
</cnode>
</pre>

It shows like:

<pre>
a {
  b {
    c {
      d val;
    }
  }
}
</pre>

But from schema stand point both are essencially same.  So either case
command path is defined as:

a b c d val

Current confsys support two level value setting. So with this definition:

<pre>
<cnode name="a">
  <cnode name="b">
    <cnode name="c" value="yes">
      <cnode name="d" value="yes">
        <cnode name="val"/>
      </cnode>
    </cnode>
  </cnode>
</cnode>
</pre>

It defines:

<pre>
a {
  b {
    c d val;
  }
}
</pre>

Some people may think this is strange.  The biggest reason of this is
easy to generate JUNOScript compatible output. Even in this case
command path is same:

a b c d val

During setting the node, if there is a intermediate leaf node and
corresponding config node does not exist, the node is also created.
In the example:

bridge-domain br0 controller 10.0.0.1

Not only the "leaf" A.B.C.D but also, WORD node has "leaf" attribute.
So "set" operation to

set bridge-domain br0 controller 10.0.0.1

will be actually two set if "bridge-domain br0" does not exist:

set bridge-domain br0
set bridge-domain br0 controller 10.0.0.1

This means in one "set" command may invoke several "set" operation
when intermediate config node does not exist.

In contrary "unset" operation will remove child node leaf
configuration as well. So unset to

unset bridge-domain br0

may actually below two "unset" operation if child config node exist.

unset bridge-domain br0 controller 10.0.0.1
unset bridge-domain br0

The child node deletion will be happening the most deepest leaf node
to parent node order.  So it traverse config tree recursively.


4) Defining and install callback

To define and install callback, we use CALLBACK() macro for function
definition.  Let's say we want to add callback function for the
command:

bridge-domain br0 controller 10.0.0.1

The schema is defined like this:

<pre>
<!-- Bridge domains -->
<cnode name="bridge-domains" multi="true" help="Bridge domains">
  <cnode name="WORD" help="Bridge name" leaf="yes">
    <cnode name="controller" help="OpenFlow controller" multi="true">
      <cnode name="A.B.C.D" help="Controller address" leaf="yes">
      </cnode>
    </cnode>
  </cnode>
</cnode>
</pre>

This command path is:

bridge-domain WORD controller A.B.C.D

So here is the way to define and install callback function for the
node for CONFSYS_MSG_TYPE_SET and CONFSYS_MSG_TYPE_UNSET operation.

<pre>
--------------------------------------------------------
CALLBACK(bridge_domain_controller_func)
{
  ARG_USED();

  /\* Argument is passed as argc/argv.  When this command is called from
   *
   * bridge-domain br0 controller 10.0.0.1
   *
   * Arguments are set as:
   *
   * argv[0] = "bridge-domain"
   * argv[1] = "br0"
   * argv[2] = "controller"
   * argv[3] = "10.0.0.1"
   *\/

  /\* Confsys message type is passed by confsys->type. *\/
  if (confsys->type == CONFSYS_MSG_TYPE_SET) {
    /\* Set operation. *\/
  } else if (confsys->type == CONFSYS_MSG_TYPE_UNSET) {
    /\* Unset operation. *\/
  } else {
    /\* Unknown type.  *\/
    return CONFIG_FAILURE;
  }
  return CONFIG_SUCCESS;
}

/\* Using global cserver pointer allocated in agent.  First argument is
 * Top cnode.  For set/unset "schema" tree it is cserver->schema.
 * Second argument is command path.  Third argument is callback
 * function pointer.  *\/
install_callback(cserer->schema,
                 "bridge-domain WORD controller A.B.C.D",
                  bridge_domain_controller_func);
--------------------------------------------------------
</pre>

Actually CALLBACK() macro is defined as

<pre>
confsys.h
--------------------------------------------------------
#define CALLBACK(callback_func_name)                                          \
int                                                                           \
callback_func_name(struct confsys *confsys, int argc, char **argv);           \
int                                                                           \
callback_func_name(struct confsys *confsys, int argc, char **argv)
--------------------------------------------------------
</pre>

So if you want to avoid using CALLBACK() macro, same function can be
defined like this:

<pre>
--------------------------------------------------------
int
bridge_domain_controller_func(struct confsys *confsys, int argc, char **argv);

int
bridge_domain_controller_func(struct confsys *confsys, int argc, char **argv)
{
  ARG_USED();
  ...
  return CONFIG_SUCCESS;
}

install_callback(cserer->schema,
                 "bridge-domain WORD controller A.B.C.D",
                  bridge_domain_controller_func);
--------------------------------------------------------
</pre>

When you want to install command for CONFSYS_MSG_TYPE_GET,
cserver->show should be used to the install nodeq.

<pre>
--------------------------------------------------------
CALLBACK(show_version_func)
{
  ARG_USED();
  show(confsys, "Version %s\n", "0.1");
  return CONFIG_SUCCESS;
}

install_callback(cserver->show,"show version", show_version_func);
--------------------------------------------------------
</pre>

Same as "exec" command.

<pre>
--------------------------------------------------------
CALLBACK(stop_process_func)
{
  ARG_USED();
  show(confsys, "Stopping process\n");
  global_state_request_shutdown(SHUTDOWN_RIGHT_NOW);
  return CONFIG_SUCCESS;
}

install_callback(cserver->exec, "stop-process", stop_process_func);
--------------------------------------------------------
</pre>

When callback function need to send back return string, it can be
handled by show() function.  As you can see in above example,
show(confsys, "", ...) takes same argument of printf(), when show()
comman is called, it format argument using vsnprintf() then store it
in output buffer.  When callback() function execution is finished, the
string is sent back to confsys client over TCP socket.


5) Confsys message

Confsys message format is as follows:

<pre>
 +------------------+------------------+-----------------
 | Type (2 octet)   | Length (2 octet) | Value (variable)
 +------------------+------------------+-----------------
</pre>

Type is defined as enum confsys_msg_type.  This type is used when
client->server message.

<pre>
------------------------------------
/\* Confsys message types.  This message type is used from client to
 * server to client message. *\/
enum confsys_msg_type {
  CONFSYS_MSG_TYPE_GET            = 0,
  CONFSYS_MSG_TYPE_SET            = 1,
  CONFSYS_MSG_TYPE_SET_VALIDATE   = 2,
  CONFSYS_MSG_TYPE_UNSET          = 3,
  CONFSYS_MSG_TYPE_UNSET_VALIDATE = 4,
  CONFSYS_MSG_TYPE_EXEC           = 5,
  CONFSYS_MSG_TYPE_LOCK           = 6,
  CONFSYS_MSG_TYPE_UNLOCK         = 7
};
------------------------------------
</pre>

Length is header length (4 octet) + value length. Communication path
is TCP over UNIX domain socket.  For server->client return message,
enum config_result_t value is used for the Type value:

<pre>
------------------------------------
/\* Confsys callback function return value.  When callback function
 * returns CONFIG_SUCCESS, corresponding confsys node is created
 * otherwise confsys node is not created.  *\/
enum config_result_t {
  CONFIG_SUCCESS = 0,
  CONFIG_FAILURE = 1,
  CONFIG_NO_CALLBACK = 2,
  CONFIG_ALREADY_EXIST = 3,
  CONFIG_DOES_NOT_EXIST = 4,
  CONFIG_SCHEMA_ERROR = 5
};
------------------------------------
</pre>

The actual UNIX domain socket path is defined in confsys.h:

<pre>
confsys.h
=========
#define CONFSYS_CONFIG_PATH "/tmp/.lagopus"
#define CONFSYS_SHOW_PATH   "/tmp/.lagopus_show"
</pre>

To set or unset the configuration, CONFSYS_MSG_TYPE_SET and
CONFSYS_MSG_TYPE_UNSET is used.

When we want to set

bridge-domain br0 controller 10.0.0.1

Type: CONFSYS_MSG_TYPE_SET
Length: 4 + strlen(bridge-domain br0 controller 10.0.0.1)
Value: bridge-domain br0 controller 10.0.0.1

When return message's Type is CONFIG_SUCCESS, the operation success,
otherwise failed reason is retured as Type value.  There is a case
that return message has Value.  That can be error message sent from
server.  So confsys client must read all of Length value from the
socket.

To get value from confsys server, CONFSYS_MSG_TYPE_GET is used.  Let's
say we want to have "bridge-domain" information for lagosh show,

show bridge-domain

should be sent. So confsys message is:

Type: CONFSYS_MSG_TYPE_GET
Length: 4 + strlen(show bridge-domain)
Value: show bridge-domain

To lock or unlock the configuration, CONFSYS_MSG_TYPE_LOCK and
CONFSYS_MSG_TYPE_UNLOCK are used.  Both value is zero so message
length is 4 (type + length).  When lock success, CONFSYS_SUCCESS is
returned otherwise CONFSYS_FAILURE is returned.  The unlock operation
success always so return value is CONFSYS_SUCCESS.  When the TCP
connection is closed, the lock is automatically unlocked.  So no need
of warry about locking the configuration forever.

To execute command in server side CONFSYS_MSG_TYPE_EXEC is used.
For "stop-process" command, confsys messge is:

Type: CONFSYS_MSG_TYPE_EXEC
Length: 4 + strlen(stop-process)
Value: stop-process

Currently CONFSYS_MSG_TYPE_SET_VALIDATE and
CONFSYS_MSG_TYPE_UNSET_VALIDATE is not yet implemented.  It is
reserved for future.


6) Sample code for confsys message.

There is a sample code which open UNIX domain socket, send message and
read message.  For actual code, please refer to:

int
confsys_client_message():

function in cclient.c.


7) Load sequence of confsys server.

When cserver is allocated, followin process is executed.

<pre>
cserver.c
=========
struct cserver *
cserver_alloc(void)
{
  Allocate struct cserver
  Client structure init.

  Load "operationsl.xml";
  Load "configuration.xml";

  Set cserver->show
  Set cserver->exec
  Set cserver->schema
  Allocate cserver->config top node

  Install cserver callbacks
}
</pre>

From agent side.  cserver load sequence is like this:

<pre>
agent.c
=======
/\* Allocate cserver. *\/
config_init();

/\* Install config and agent callbacks. *\/
config_install_callback();
agent_install_callback();

/\* Load config from lagopus.conf. *\/
config_load_lagopus_conf();
</pre>

Please note that config_load_lagopus_conf() *does not* use lagosh.
Injection of configuration must not have any dependencies to specific
client.  In config_load_lagopus_conf() function, it opens lagopus.conf
by itself and parse saved configuration.  So when system manager want
to EMS/NSM instead of file based configuration or system manager want
to use OF-Config with NetConf infrastructure, they can avoid using
this file based initial configuration at all.


8) lagosh

lagosh is command line interface which follows the Juniper CLI
concept.  lagosh has two mode one is operational mode one is
configuration mode.  When lagosh is launched, the shell is launched in
operational mode.

<pre>
$ ./lagsh
lagopus>
</pre>

When type '?' it shows possible candidate commands.

<pre>
lagopus> ?
Possible completions:
  configure      Manipulate software configuration information
  exit           Exit the management session
  help           Provide help information
  history        Show command history
  ping           Ping a remote target
  quit           Exit the management session
  show           Show information about the system
  ssh            Open a secure shell to another host
  start          Start command
  telnet         Telnet to another host
  traceroute     Trace the route to a remote host
</pre>

Same as normal shell, we can exectute command such as "ping", "traceroute"

<pre>
lagopus> ping localhost
PING localhost (127.0.0.1) 56(84) bytes of data.
64 bytes from localhost (127.0.0.1): icmp_req=1 ttl=64 time=0.037 ms
64 bytes from localhost (127.0.0.1): icmp_req=2 ttl=64 time=0.035 ms
^C
--- localhost ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1000ms
rtt min/avg/max/mdev = 0.035/0.036/0.037/0.001 ms
</pre>

Type Control-C will terminate the command execution.
To switch to configuration mode. "configure" command is used.

<pre>
lagopus> configure
Entering configuration mode
[edit]
</pre>

Here we can show current configuration with "show" command.

<pre>
lagopus> show
bridge-domains {
    br1 {
        controller {
            127.0.0.1;
        }
    }
}
protocols {
    openflow {
        traceoptions {
            file ofp.log;
        }
    }
}
[edit]
</pre>

In configuration mode we can set or delete(unset) the configuration.

<pre>
lagopus> set protocols openflow traceoptions flag hello;
lagopus> show
bridge-domains {
    br1 {
        controller {
            127.0.0.1;
        }
    }
}
protocols {
    openflow {
        traceoptions {
            file ofp.log;
            flag hello;
        }
    }
}
</pre>

Same as delete

<pre>
lagopus> delete protocols openflow traceoptions flag hello;
lagopus> show
bridge-domains {
    br1 {
        controller {
            127.0.0.1;
        }
    }
}
protocols {
    openflow {
        traceoptions {
            file ofp.log;
        }
    }
}
</pre>

The configuration can be saved to lagopus.conf with "save" command

<pre>
lagopus> save

Configuration is saved!

lagopus>
</pre>

lagosh is just simply a shell.  It does not parse lagopus.conf neither
store any configuration in it.  All of those functionality is taken
care by cserver not by lagosh.

*/

#include <stdint.h>
#include "lagopus_apis.h"
#include "lagopus_config.h"
#include "lagopus_types.h"
#include "lagopus_thread.h"
#include "lagopus_gstate.h"

/* Forward declaration. */
struct confsys;
struct cserver;
struct clink;
struct cnode;
struct event_manager;

/**
 * Callback function define macro.
 *
 * @param[in]	callback_func_name
 *
 * @retval CONFIG_SUCCESS  Successded.
 * @retval CONFIG_FAILURE  Failed.
 */
#define CALLBACK(callback_func_name)                                          \
  int                                                                           \
  callback_func_name(struct confsys *confsys, int argc, const char **argv);     \
  int                                                                           \
  callback_func_name(struct confsys *confsys, int argc, const char **argv)

/* For supressing compilation warnings. */
#define ARG_USED()                                                            \
  do {                                                                        \
    (void)confsys;                                                            \
    (void)argc;                                                               \
    (void)argv;                                                               \
  } while (0);

/* Typedef for callback function type. */
typedef int callback_t(struct confsys *confsys, int argc, const char **argv);

/**
 * Install callback function to the str's confsys path node.
 *
 * @param[in]	top	Top node of the cnode tree.
 * @param[in]	str	cnode path which the callback to be installed.
 * @param[in]	callback	Callback function pointer.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Failed.  The cnode path node does not exists.
 */
lagopus_result_t
install_callback(struct cnode *top, const char *str, callback_t callback);

/**
 * Install server side config callback function to the str's confsys path node.
 *
 * @param[in]	str	cnode path which the callback to be installed.
 * @param[in]	callback	Callback function pointer.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_NOT_STARTED	Failed, cserver is not started.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Failed.  The cnode path node does not exists.
 */
lagopus_result_t
install_config_callback(const char *str, callback_t callback);

/**
 * Install server side exec callback function to the str's confsys path node.
 *
 * @param[in]	str	cnode path which the callback to be installed.
 * @param[in]	callback	Callback function pointer.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_NOT_STARTED	Failed, cserver is not started.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Failed.  The cnode path node does not exists.
 */
lagopus_result_t
install_exec_callback(const char *str, callback_t callback);

/**
 * Install server side show callback function to the str's confsys path node.
 *
 * @param[in]	str	cnode path which the callback to be installed.
 * @param[in]	callback	Callback function pointer.
 *
 * @retval LAGOPUS_RESULT_OK		Succeeded.
 * @retval LAGOPUS_RESULT_NOT_STARTED	Failed, cserver is not started.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Failed.  The cnode path node does not exists.
 */
lagopus_result_t
install_show_callback(const char *str, callback_t callback);

/* Confsys message types.  This message type is used from client to
 * server message.  For server return message, type is enum
 * config_result_t */
enum confsys_msg_type {
  CONFSYS_MSG_TYPE_GET            = 0,
  CONFSYS_MSG_TYPE_SET            = 1,
  CONFSYS_MSG_TYPE_SET_VALIDATE   = 2,
  CONFSYS_MSG_TYPE_UNSET          = 3,
  CONFSYS_MSG_TYPE_UNSET_VALIDATE = 4,
  CONFSYS_MSG_TYPE_EXEC           = 5,
  CONFSYS_MSG_TYPE_LOCK           = 6,
  CONFSYS_MSG_TYPE_UNLOCK         = 7
};

/* Confsys callback function return value.  When callback function
 * returns CONFIG_SUCCESS, corresponding confsys node is created
 * otherwise confsys node is not created.  This is used for server
 * to client return message type as well. */
enum config_result_t {
  CONFIG_SUCCESS = 0,
  CONFIG_FAILURE = 1,
  CONFIG_NO_CALLBACK = 2,
  CONFIG_ALREADY_EXIST = 3,
  CONFIG_DOES_NOT_EXIST = 4,
  CONFIG_SCHEMA_ERROR = 5
};

/* Argument to the callback function. */
struct confsys {
  /* Message type. */
  enum confsys_msg_type type;

  /* Server pointer. */
  struct cserver *cserver;

  /* Client link. */
  struct clink *clink;
};

/* Config system message header. */
struct confsys_header {
  uint16_t type;
  uint16_t length;
  uint8_t value[0];
};

/* Size of struct confsys_header (4 octet). */
#define CONFSYS_HEADER_LEN   sizeof(struct confsys_header)

/* Confsys server socket. */
#define CONFSYS_CONFIG_SOCKNAME   ".lagopus"
#define CONFSYS_SHOW_SOCKNAME     ".lagopus_show"
#define CONFSYS_CONFIG_PATH   confsys_sock_path(CONFSYS_CONFIG_SOCKNAME)
#define CONFSYS_SHOW_PATH     confsys_sock_path(CONFSYS_SHOW_SOCKNAME)

/* Confsys default load path. */
#define CONFSYS_LOAD_PATH     CONFDIR

/* Confsys configuration file name. */
#define CONFSYS_CONFIG_FILENAME       "lagopus.conf"
#define CONFSYS_CONFIG_FILE   CONFDIR "/lagopus.conf"
#define CONFSYS_CONFIG_DEFAULT_PATH   "/etc/lagopus"
#define CONFSYS_CONFIG_DEFAULT_FILE   CONFSYS_CONFIG_DEFAULT_PATH "/lagopus.conf"

/* Confsys group. */
#define CONFSYS_GROUP_NAME    "lagopus"

const char *
confsys_sock_path(const char *basename);

/**
 * Load confsys schema file then return top cnode pointer.
 *
 * @param[in]	filename	Filename of the schema file.
 *
 * @retval NULL	Schema file load error.
 * @retval Non-Null Top cnode pointer of the loaded schema.
 */
struct cnode *
confsys_schema_load(const char *filename);

/**
 * Load lagopus.conf file and invoke callback functions in the
 * configuration.  location of lagopus.conf is ${sysconfdir}/lagopus.
 * Default is /usr/local/etc/lagopus.
 *
 * @param[in]	cserver
 *
 * @retval LAGOPUS_RESULT_OK	Succeeded.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Fialed, lagopus.conf file not found.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, parse error.
 *
 */
int
cserver_load_lagopus_conf(struct cserver *cserver);

/**
 * Parse lagopus.conf again and invoke callback functions in the
 * configuration.  location of lagopus.conf is ${sysconfdir}/lagopus.
 * Default is /usr/local/etc/lagopus.
 *
 * @param[in]	cserver
 *
 * @retval LAGOPUS_RESULT_OK	Succeeded.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Fialed, lagopus.conf file not found.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 * @retval LAGOPUS_RESULT_INVALID_ARGS	Failed, parse error.
 *
 */
int
cserver_propagate_lagopus_conf(struct cserver *cserver);

/**
 * Config APIs.
 *
 * API starts from config_* uses singleton confsys structure.  So no
 * need of specify struct confsys * as first argument.
 *
 */

/**
 *
 *
 */
lagopus_result_t
config_handle_initialize(void *arg, lagopus_thread_t **thdptr);

void
config_install_callback(void);

lagopus_result_t
config_load_lagopus_conf(void);

int
config_propagate_lagopus_conf(void);

struct cserver *
cserver_get(void);

lagopus_result_t
config_handle_start(void);

/**
 * Get cnode for the cnode path.  When no cnode is configured and
 * default value exists in schema, return cnode which has default
 * value.
 *
 * @param[in]	cserver	cserver which has config and schema.
 * @param[in]	cnode path for the value.
 *
 * @retval NULL No cnode nor default value configured.
 * @retval cnode cnode which has configuration value or default value for the cnode path.
 */
char *
config_get(const char *path);

/**
 * Set default value for the cnode.
 *
 * @param[in]	top	Top node of the config.
 * @param[in]	path cnode path for the value.
 * @param[in]	value Deafult value to be set.
 *
 * @retval LAGOPUS_RESULT_OK	Succeeded.
 * @retval LAGOPUS_RESULT_NOT_FOUND	Fialed, path does not exist.
 * @retval LAGOPUS_RESULT_NO_MEMORY	Failed, no memory.
 */
int
config_set_default(const char *path, char *val);

/**
 * Set configuration node.
 */
int
config_set(const char *path);

/**
 * Shutdown cserver according to timeout value(nsec).
 */
lagopus_result_t
config_handle_shutdown(shutdown_grace_level_t level);

/**
 * Stop immediately cserver.
 */
lagopus_result_t
config_handle_stop(void);

/**
 * Finalize mothod for confsys.
 */
void
config_handle_finalize(void);

/*
 * Include other confsys headers.  When we need confsys functions,
 * please just #include "confsys.h"
 */
#include "cnode.h"
#include "cxml.h"
#include "clex.h"
#include "cparse.h"
#include "cclient.h"
#include "cserver.h"
#include "clink.h"
#include "show.h"
#include "validate.h"

/* Thread shutdown parameters. */
#define TIMEOUT_SHUTDOWN_RIGHT_NOW     (100*1000*1000) /* 100msec */
#define TIMEOUT_SHUTDOWN_GRACEFULLY    (1500*1000*1000) /* 1.5sec */

#endif /* SRC_CONFIG_CONFSYS_H_ */
