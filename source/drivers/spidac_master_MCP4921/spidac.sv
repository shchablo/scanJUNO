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

module spidac
#(
	parameter DATA_WIDTH=8, parameter ADDR_WIDTH=5 
 )
(
 input wire clk,
 input wire init, input wire res,
 input wire we, input wire [DATA_WIDTH-1:0]addr,
 input wire [DATA_WIDTH-1:0]data_in,
 output wire [DATA_WIDTH-1:0]data_out,
 
 output reg SCK,
 output reg nCS,
 output reg nLDAC,
 output reg SDI,
 
 input wire start_step,
 output reg start,
 
 input wire s_ready_counter_start,
 
 output reg run
);

wire clk_40Mhz;
reg [DATA_WIDTH-1:0]ram[ADDR_WIDTH-1:0];

wire [15:0]data;

reg load;
assign data[7:0] = ram[1];
assign data[15:8] = ram[2];
integer counter_load;

wire [15:0]code;
assign code[7:0] = ram[3];
assign code[15:8] = ram[4];

wire wRun;
//reg run;
assign wRun = run;
reg s_ready_write_dac;
reg switch_start_step;
always @ (start_step or s_ready_write_dac or init) begin 
	if(init) begin
		switch_start_step <= 1;
		run <= 0;
	end	
	if(start_step == 0)
		switch_start_step <= 1;

	if(start_step == 1 && s_ready_write_dac == 1) begin
		if(switch_start_step == 1) begin
			run <= 1;
			switch_start_step <= 0;
		end
	end
	else
		run <= 0;
end

reg switch_nLDAC;
always @ (nLDAC or s_ready_counter_start or init) begin 
	if(init) begin
		switch_nLDAC = 1;
		start = 0;
	end	
	if(nLDAC == 1)
		switch_nLDAC = 1;

	if(nLDAC == 0 && s_ready_counter_start == 1) begin
		if(switch_nLDAC == 1) begin
			start = 1;
			switch_nLDAC = 0;
		end
	end
	else
		start = 0;
end

always @ (posedge clk) begin 

// initialization
	if(init) begin
	   s_ready_write_dac <= 1;
		code <= 0;
		ram[0] <= 8'b00000000;
		ram[1] <= 0;
		ram[2] <= 0;
		load <= 0;
		counter_load <= 0;
	end
	
  if(wRun) begin
		ram[0] <= 8'b00000010;
		s_ready_write_dac <= 0;
	end
  
  if(ram[0] == 8'b0000010) begin
		counter_load <= 2;
		ram[0] <= 8'b00000100;
	end
	if(counter_load > 0 && ram[0] == 8'b00000100) begin
		load <= 1;
		counter_load <= counter_load - 1;
	end
	if(counter_load == 0 && ram[0] == 8'b00000100) begin
		load <= 0;
		s_ready_write_dac <= 1;
		ram[0] <= 8'b00000000;
	end
  
  // write memory	
	if(we) begin
		case(addr)
			8'h02:  ram[3] <= data_in;
			8'h03:  ram[4] <= data_in;
			
			8'h23:  ram[0] <= data_in;
			8'h24:  ram[1] <= data_in;
			8'h25:  ram[2] <= data_in;
		endcase
	end
		
	// read memory  			
	case(addr)
		8'h02: 	data_out <= ram[3];
		8'h03:   data_out <= ram[4];
		8'h23:	data_out <= ram[0];
		8'h24:   data_out <= ram[1];
		8'h25:   data_out <= ram[2];
	endcase
	
	// Command: Reset 
	if(ram[0] == 8'b00000001 || res) begin
		ram[0] <= 0;
		ram[1] <= 0;
		ram[2] <= 0;
		load <= 0;
		counter_load <= 0;
	end
end

altpll_m4_d5 altpll_m4_d5_inst
(
	clk,
	clk_40Mhz
);
// Instantiation SPIDAC_MASTER_MCP4921. Signals and registers declared.	
SPIDAC_MASTER_MCP4921 SPIDAC_MASTER_MCP4921_inst
	(
		clk_40Mhz,
		data[11:0],
		load,
		
		SCK, 
		nCS,
		nLDAC,
		SDI
	);

endmodule