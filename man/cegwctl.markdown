% CAN-ETH-GW(8) CAN - Ethernet Gateway Control Utility User Manual | master-preview
% Fabian Raab <fabian.raab@tum.de>
% July 31, 2014

# NAME

cegwctl - CAN - Ethernet Gateway Control Utility


# SYNOPSIS

**cegwctl** [ **-f** | **\--can-fd** ] [ **-t** *TYPE* | **\--type**=*TYPE* ] [ **-b** | **\--bidirectional** ] **add** **route** *SRC* *DST*

**cegwctl** [ **-f** | **\--can-fd** ] [ **-t** *TYPE* | **\--type**=*TYPE* ] **add** **dev** [*NAME*]

*TYPE* := { **none** | **eth** | **net** | **udp** | **tcp** }

**cegwctl** **del** **route** *ID*

**cegwctl** **del** **dev** *NAME*

**cegwctl** **route**

# DESCRIPTION

Control Utility for the `ce_gw`  Kernel Programm.



# OPTIONS

**-b**, **\--bidirectional**
:	If you add a route from SRC to DST, there will be also add the same route back from DST to SRC.

**-f**, **\--can-fd**
:	A Flag for the larger CAN-FD Frames.

**-t**, **\--type**=*TYPE*
:	Set The Type of the Gateway. '**net**' is the Default. **udp**, **tcp** are not supported yet.

*TYPE* := { **none** | **eth** | **net** | **udp** | **tcp** }
:	Types

# COMMANDS

**add route** *SRC* *DST*

:	Add a Gateway from *SRC* to *DST*, where SRC and DST are the String names of the Interfaces.

**add dev** [*NAME*]

:	Add a virtual ethernet device. You need this device for adding a route. *NAME* will be the name of the new device. If you don't set *NAME*, it will be called \`cegw%d\`, where %d is a consecutive number. With the options [ **-f** | **\--can-fd** ] [ **-t** *TYPE* | **\--type**=*TYPE* ] you can set a meaningfull MTU.


	| TYPE | FD?     | BYTE | MTU Size Description     |
	|------|:-------:|------|--------------------------|
	| NONE |    -    | 1500 | Standart Ethernet MTU    |
	| ETH  | &cross; | 8    | Max CAN Payload          |
	| ETH  | &#10004;| 64   | Max CAN-FD Payload       |
	| NET  | &cross; | 16   | Max CAN Frame            |
	| NET  | &#10004;| 72   | Max CAN-FD Frame         |
	| UDP  |    -    | 1500 | Standart Ethernet MTU    |
	| TCP  |    -    | 1500 | Standart Ethernet MTU    |


**del route** *ID*
:	Deletes the Gateway with *ID*.

**del dev** *NAME*
:	Deletes a previously with **add** **dev** added device with *NAME* as name.

**route** [*ID*]
:	List active Gateways and Informations or if *ID* is specified, Information of the Gateway with *ID* will printed

# EXAMPLES

#### Add a Gateway:

	cegwctl -f -t net add route "eth0" "can0"



# EXIT STATUS

	| CODE | Description							|
	|:----:|----------------------------------------|
	|  0   | success								|
	| !=0  | failure 								|

# COPYRIGHT
&copy; Copyright 2013 Fabian Raab, Stefan Smarzly

This file is part of CAN-Eth-GW.

CAN-Eth-GW is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAN-Eth-GW is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with CAN-Eth-GW. If not, see [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/).

# SEE ALSO

**Homepage:** [http://can-eth-gw.github.io](http://can-eth-gw.github.io)

ip(8)
