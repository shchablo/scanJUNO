Program: scanJUNO
--------------------------------------------------------------------------------

Description:

   FPGA system for scan station for Photomultiplier from JINO experiment.
--------------------------------------------------------------------------------

Content:
   
  quartus_project	: Folder for quartus project.
  source		: Folder for FPGA source files.
  linux			: Folder for tcl script jtag interface.
  qsys/cpu_eth		: Folder for NIOSII files and C code for web server.
			  sopc_onchip_mem.hex - should de located in quartus_project during compilation FPGA project.
  quartus_project       : Main folder for project.

  LICENSE.txt
  LICENSE_rus.txt
  NOTICE.txt
  README.txt

 .git
--------------------------------------------------------------------------------

Software:
	Quartus II 13.1 (64-bit) Web Editionl Build 162;
	Qsys 13.1 Build 162;
	Eclipse IDE for C/C++ Developers Version: Indigo Service Release 2 Build id: 20120216-1857.

--------------------------------------------------------------------------------

FPGA: Cyclone 3 EP3C16F4844C6

Task: For terasIC Altera DE0 Board 
(Link: https://yadi.sk/i/qwQsqiwB3D8Gvc)
--------------------------------------------------------------------------------

UDP: 

If you want to work with UDP you must use 1200 port. default ip: 192.168.1.4


Command:

1. "/help" - ANSWER "Information:

1. "/hi" - ANSWER "Hello".

3. "/dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]&" 
	
	ANSWER "Ok" or "Please wait ~%d sec or interrupt run!".
 	
	Example: "/dac=[0]&time=[1]&step=[1]&nSteps=[4095]&"

4. "/dac=[0-4095code]&time=[0-255sec]&step=[0-4095code]&nSteps=[0-4095]&calb&" 
	
	ANSWER "Ok" or "Please wait ~%d sec or interrupt run!".
	
	Example: "/dac=[0]&time=[1]&step=[1]&nSteps=[4095]&calb&"


5. "/freq=[0-2^32code]&"

6. "/rfreq"

7. "/gate=[0-10]&"

8. "/rgate"


9. "/interrupt" -  ANSWER "nSteps = 0, Ok, please wait a few second".


10. "/addr=[0-255]&"

11. "/data=[0-255]&"

12. "/rdata"

--------------------------------------------------------------------------------

Support:

  Information about the project: https://yadi.sk/d/_KovRU8Z3CgPTo (Download and run in browser) - Russia. 

  ShchabloKV@gmail.com (8-906-796-76-53)
--------------------------------------------------------------------------------

