Program: scanJUNO
--------------------------------------------------------------------------------

Description:

   FPGA system for scan station for Photomultiplier.
--------------------------------------------------------------------------------

Content:
   
  quartus_project	: Folder for quartus project.
  source		: Folder for FPGA source files.
  linux			: Folder for tcl script jtag interface.
  qsys/cpu_eth		: Folder for NIOSII files and C code for web server.
			  sopc_onchip_mem.hex - should be located in quartus_project folder during compilation FPGA project.
  quartus_project       : Main folder for project.

  LICENSE.txt
  LICENSE_rus.txt
  NOTICE.txt
  README.txt
  TEST.txt

 .git
--------------------------------------------------------------------------------

Software:
	Quartus II 13.1 (64-bit) Web Editionl Build 162;
	Qsys 13.1 Build 162;
	Eclipse IDE for C/C++ Developers Version: Indigo Service Release 2 Build id: 20120216-1857;
	---
	Windows Service Pack 1.

--------------------------------------------------------------------------------

FPGA: Cyclone 3 EP3C16F4844C6

Task: For terasIC Altera DE0 Board 
(Link: https://yadi.sk/i/qwQsqiwB3D8Gvc)
--------------------------------------------------------------------------------

Interfaces:

Description for tcl script interface and web interface you can find in the wiki (https://yadi.sk/d/_KovRU8Z3CgPTo).


UDP: 

If you want to work with UDP you must use 1200 port. default ip: 192.168.1.4

Examples:

1) "\hi";
   "\dac=0&time=1&step=1&nSteps=4095&calb=1&";
   "\dac=0&time=1&step=1&nSteps=4095&".

2) "\hi";
   "\dac=0&time=1&step=1&nSteps=4095&calb=1&";
   "\freq=10&";
   "\dac=0&time=1&step=1&nSteps=4095&";
   "\interrupt".


Commands:

1. "\hi" - ANSWER "hi".

1. "\help" - ANSWER "Information: ..."

3. "\dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]&" 
	
	ANSWER "Ok" or "Please wait ~%d sec or interrupt run!". 
	
	DURING RUN: "[dac, count]"
 	
	Example: "/dac=[0]&time=[1]&step=[1]&nSteps=[4095]&"

4. "\dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]&calb=1&" 
	
	ANSWER "Ok" or "Please wait ~%d sec or interrupt run!".

	DURING RUN: "[dac, count]" and "[dac]"
	
	Example: "/dac=[0]&time=[1]&step=[1]&nSteps=[4095]&calb=1&"


5. "\freq=[0-2^32code]&" - ANSWER "[value]"

6. "\rfreq" - ANSWER "[value]"

7. "\gate=[0-10]&" - ANSWER "[value]"

8. "\rgate" - ANSWER "[value]"

9. "\addr=[0-255]&" - ANSWER "[value]"

8. "\raddr" - ANSWER "[value]"

10. "\data=[0-255]&" - ANSWER "[value]"

11. "\rdata" - ANSWER "[value]"

12. "\interrupt" -  ANSWER "nSteps = 0, Ok, please wait a few second".

--------------------------------------------------------------------------------

Support:

  Information about the project (wiki): https://yadi.sk/d/_KovRU8Z3CgPTo (Download and run in browser) - Russian language. 

  ShchabloKV@gmail.com (8-906-796-76-53)
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------

Support:

  Information about the project: https://yadi.sk/d/_KovRU8Z3CgPTo (Download and run in browser) - Russia. 

  ShchabloKV@gmail.com (8-906-796-76-53)
--------------------------------------------------------------------------------

