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

module eth_top(
		
	input  wire  reset_n,        
	output wire  out_port_from_the_LAN_RST,
	output wire  out_port_from_the_LAN_CS,  
	input  wire  MISO_to_the_LAN,           
	output wire  MOSI_from_the_LAN,        
	output wire  SCLK_from_the_LAN,        
	
	input  wire  clk,                       
	
	input  wire  in_port_to_the_LAN_NINT,   
	
	input  wire [31:0]time_export,               
	input  wire [31:0]signals_export,  
	          
	output wire [7:0]addr_export,             
	input  wire [7:0]rdata_export,              
	output wire [7:0]wdata_export,              
	
	output wire		 swrite_export,             
	output wire      sread_export,             
	output wire		 cread_export,  // counter read signal 
	output wire		 addr_write_export,  // addr wirite signal         
	
	/* signals for start and stop run */
	output wire	startStep_export,
	input wire	stopStep_export,
	
	output wire  		swrite32_export,
	output wire [31:0]  wdata32_export,
	input  wire [31:0]  rdata32_export 
	);

wire  SS_n_from_the_LAN;
wire  dclk_from_the_epcs; 
wire  sce_from_the_epcs;
wire  sdo_from_the_epcs;
wire  data0_to_the_epcs; 

wire	[7:0]signals_export_array;  
wire	[7:0]status_export_array;       		

assign status_export_array[4] = stopStep_export;

assign rwrite32_export = signals_export_array[6];
assign swrite32_export = signals_export_array[5];
assign startStep_export = signals_export_array[4];
assign swrite_export = signals_export_array[3];
assign sread_export = signals_export_array[2];
assign cread_export = signals_export_array[1];
assign addr_write_export = signals_export_array[0];

sopc sopc_inst (
		
	reset_n,           
	
	out_port_from_the_LAN_RST,
	out_port_from_the_LAN_CS,
	MISO_to_the_LAN,      
	MOSI_from_the_LAN,       
	SCLK_from_the_LAN,         
	SS_n_from_the_LAN,      
	
	clk,                      
	in_port_to_the_LAN_NINT,   
	
	dclk_from_the_epcs,      
	sce_from_the_epcs,       
	sdo_from_the_epcs,    
	data0_to_the_epcs,
			
	time_export,        
	signals_export,          
	addr_export,         
	rdata_export,            
	
	wdata_export,      
	
	signals_export_array,
	status_export_array,
	
	wdata32_export,
	rdata32_export
);

endmodule