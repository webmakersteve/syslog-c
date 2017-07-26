# syslog.c - Fast Syslog parser written in C

Copyright (c) Stephen Parente

__syslog.c__ is a fast RFC5424 syslog message parser written in C. It can parse structured data and regular messages into an easy to use struct for your application.

It is licensed under the GPL.

__Note:__ This is not under active development. If bugs are found, I would be happy to take a pull request to fix them, or I may fix them in my spare time - but it is not a huge active priority for me.

## Overview

The library provides an interface for parsing a `char*` into a `syslog_message_t`. See https://github.com/webmakersteve/syslog.c/blob/master/src/syslog.h for the interfaces.

## Parsing a Syslog message

```c
syslog_message_t msg = {};
char * mm = "<165>1 2016-12-16T12:00:00.000Z hostname appname PROCID MSGID Logging message...";
if (!parse_syslog_message_t(mm, &msg)) {
  return 1;
}

# If it succeeded, msg will be filled in. 

# msg.severity = 5
# msg.facility = 20
# msg.pri_value = 165
# msg.syslog_version = "1"

# msg.message_id = "MSGID"
# msg.hostname = "hostname"
# msg.appname = "appname"
# msg.process_id = "PROCID"

# Destroy it when you are done
# This frees the internal buffer and structured element pointers. You are still responsible for getting rid of
# the message out parameter if it is heap allocated

free_syslog_message_t(&msg);
```

The vast majority of this library is vested in one function: `parse_syslog_message_t` 

## Installing

To build and install the library run this:

```sh
make
make test
make install
```

There should be no outside dependencies required. This will install the library to your system. 

To link with it, just provide `-lsyslog` to your linker flags and include the header in files that need it

`#include <webmakersteve/syslog.h>`

## Performance

Performance is of the utmost importance. Any PRs that increase the result of the benchmark scripts reliably will be taken into serious consideration! To run the benchmarks:

```sh
make
make benchmark
./benchmark
```

It will typically take about 15-20 seconds to complete. The library is currently capable of parsing around 1.8 million syslog entries per second without structured data, and around 600,000 with one structured data group with two elements on my machine (just a macbook pro).
