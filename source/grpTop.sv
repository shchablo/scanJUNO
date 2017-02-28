//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. 
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module grpTop(
  
	input clock50Mhz,
  
	input [9:0]keys_gate,
	input key_restart,
  
	output [3:0]gate_out,
  
	// light indicator
	output [9:0]gate_led,
	output [7:0]address_hex0,
	output [7:0]address_hex1,
	output [7:0]address_hex2,
	output [7:0]address_hex3,
  
	output SCK, 
	output nCS,
	output nLDAC,
	output SDI,
	
	output wire  out_port_from_the_LAN_RST,
	output wire  out_port_from_the_LAN_CS, 
	input  wire  MISO_to_the_LAN,          
	output wire  MOSI_from_the_LAN,        
	output wire  SCLK_from_the_LAN,        
	input  wire  in_port_to_the_LAN_NINT,  
	
	output pwm_out,
	output [9:0]ch_pwm_out,
	
	input count,
	
	output t_SCK,
	output t_nCS,
	output t_nLDAC,
	output t_SDI
	
  );
  
// Instantiation initializer. Signals and registers declared.
wire init;
wire reset;
initializer initializer_inst
(
	clock50Mhz,
	init
);
		
wire [7:0]data_in;

// Instantiation vjtag. Signals and registers declared.
wire [7:0]data_vjtag_out;
wire [7:0]address_vjtag;
wire addr_write_vjtag;
wire write_vjtag;
wire addr_read_vjtag;
vjtag vjtag_inst_interface
(
	init,
	data_in, 
	data_vjtag_out, 
	address_vjtag,
	write_vjtag,
	addr_write_vjtag
);

// Instantiation eth. Signals and registers declared.
wire [31:0]time_export;
wire [31:0]signals_export;

wire [7:0]addr_export;

		
wire [7:0]wdata_export;
wire        swrite_export;
wire        sread_export; 
		
wire        cread_export;          
wire        addr_write_export;

wire        startStep_export;
wire        stopStep_export;
wire        swrite32_export;
wire [31:0]wdata32_export;
wire [31:0]rdata32_export;

eth_top eth_inst 
(
	key_restart,
	out_port_from_the_LAN_RST,
	out_port_from_the_LAN_CS,
	MISO_to_the_LAN,
	MOSI_from_the_LAN,
	SCLK_from_the_LAN,	
	clock50Mhz,		
	in_port_to_the_LAN_NINT,
	
	time_export,
	signals_export,
	addr_export,
	data_in,
		
	wdata_export,
	swrite_export,
	sread_export,
		
	cread_export,   
	addr_write_export,
	
	startStep_export,
	stopStep_export,
	
	swrite32_export,
	wdata32_export,
	rdata32_export

);

// Instantiation command. Signals and registers declared.
wire write;
wire write32;
wire [7:0]data;
wire [7:0]addr;
command command_inst
(
	clock50Mhz, key_restart,
	init, reset,
	
	addr_write_vjtag,
	address_vjtag,
	
	addr_write_export,
	addr_export,
	
	addr,
	
	write_vjtag,
	swrite_export,
	
	write,
	
	wdata_export,
	data_vjtag_out,
	
	data
);	

// Instantiation PWM. Signals and registers declared.
wire [7:0]data_pwm;
wire gen;
wire pwm_test; // test signal - should be
pwm pwm_inst(
	clock50Mhz,
	addr,
	data, data_pwm,
	write, init, reset,

	swrite32_export,
   wdata32_export,
	
	gen
);
// Instantiation counter. Signals and registers declared.
wire [7:0]data_counter;
wire start_dac;
wire s_ready_counter_start;
counter counter_inst
 (
	clock50Mhz,
	addr,
	data, data_counter,
	write, init, reset,
	
	cread_export,
	time_export,
	signals_export,
	
	start_dac,
	s_ready_counter_start,
	stopStep_export,
	
	count
);

// Instantiation gate. Signals and registers declared.
wire [7:0]data_gate;
gate gate_inst(
 	clock50Mhz,
	addr,
	data, data_gate,
	write, init, reset,
  
  gate_out,
  keys_gate,
  gate_led,
  
  gen,
  ch_pwm_out,
  pwm_out
  );
// Instantiation DAC. Signals and registers declared.
wire [7:0]data_DAC;
 spidac spidac_inst
(
 clock50Mhz,
 init, reset,
 write, addr,
 data,
 data_DAC,
 
 SCK,
 nCS,
 nLDAC,
 SDI,
 
 startStep_export,
 start_dac,
 s_ready_counter_start
);

// Instantiation addr_indicators. Signals and registers declared.
indicator16 indicator16_hex0(
 addr[3:0],
 address_hex0
);
indicator16 indicator16_hex1(
 addr[7:4],
 address_hex1
);		
	assign address_hex3[7:0] = 8'b11000000;
	assign address_hex2[7:0] = 8'b00001001;
	
	assign t_SDI = SDI;
	assign t_SCK = SCK;
	assign t_nCS = nCS;
	assign t_nLDAC = nLDAC;

reg [7:0]version;
assign version[7:0] = 8'b00010000;

// Instantiation selector. Signals and registers declared.
selector selector_data_vjtag_in(
 addr,
 data_gate,
 data_counter,
 data_pwm,
 version,
 data_DAC,
 
 data_in
);
endmodule