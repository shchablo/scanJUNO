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

module counter
#(
	parameter DATA_WIDTH=8, parameter ADDR_WIDTH=10
 )
 (
	input wire clk,
	input wire [DATA_WIDTH-1:0]addr,
	input wire [DATA_WIDTH-1:0]data_in, output wire [DATA_WIDTH-1:0]data_out,
	input wire we, input wire init, input wire res,
	
	input read_open,
	output wire [31:0]counter_time_ex,
	output wire [31:0]data_ex,
	input wire signal
);

reg [DATA_WIDTH-1:0]ram[ADDR_WIDTH-1:0];
reg flag;
reg [31:0]count_time;
reg [31:0]data;

assign counter_time_ex[7:0] = ram[6];
assign counter_time_ex[15:8] = ram[7];
assign counter_time_ex[23:16] = ram[8];
assign counter_time_ex[31:24] = ram[9];

assign data_ex[7:0] = ram[2];
assign data_ex[15:8] = ram[3];
assign data_ex[23:16] = ram[4];
assign data_ex[31:24] = ram[5];

reg read_close;

always @ (posedge clk) begin 
	
	// initialization
	if(init) begin
		ram[0] <= 0;
		ram[1] <= 0;
		
		ram[2] <= 0;
		ram[3] <= 0;
		ram[4] <= 0;
		ram[5] <= 0;
		
		ram[6] <= 0;
		ram[7] <= 0;
		ram[8] <= 0;
		ram[9] <= 0;
		
		flag <= 0;
		data <= 0;
		count_time <= 0;
		
		read_close <= 0;
	end
	
	// write memory	
	if(we) begin
		case(addr)
			8'h26:  ram[0] <= data_in;
			8'h27:  ram[1] <= data_in;
		  
			8'h28:  ram[2] <= data_in;
			8'h29:  ram[3] <= data_in;
			8'h30:  ram[4] <= data_in;
			8'h31:  ram[5] <= data_in;
	    
			8'h32:  ram[6] <= data_in;
		   8'h33:  ram[7] <= data_in;
			8'h34:  ram[8] <= data_in;
			8'h35:  ram[9] <= data_in;			
	  	endcase
	end
	
	case(addr)
		8'h26:  data_out <= ram[0];
		8'h27:  data_out <= ram[1];
	  
		8'h28:  data_out <= ram[2];
		8'h29:  data_out <= ram[3];
		8'h30:  data_out <= ram[4];
		8'h31:  data_out <= ram[5];
	  
		8'h32:  data_out <= ram[6];
		8'h33:  data_out <= ram[7];
		8'h34:  data_out <= ram[8];
		8'h35:  data_out <= ram[9];	
	endcase
	
	// Command: Reset 
	if(ram[0] == 8'b00000001 || res) begin
		ram[0] <= 0;
		ram[1] <= 0;
  	  
		ram[2] <= 0;
		ram[3] <= 0;
		ram[4] <= 0;
		ram[5] <= 0;
  	  
		ram[6] <= 0;
		ram[7] <= 0;
		ram[8] <= 0;
		ram[9] <= 0;
  	  
  	   flag <= 0;
  	   data <= 0;
  	   count_time <= 0; 
	end
	
	// Command: Read from eth.
	if(!read_open) begin
		read_close <= 0;
	end
	if(read_open && !read_close && ram[0] == 8'b00000000) begin
		read_close <= 1;
		ram[0] <= 8'b00000010;
	end
	
	if(ram[0] == 8'b00000010) begin
		ram[2] <= data[7:0];
		ram[3] <= data[15:8];
		ram[4] <= data[23:16];
		ram[5] <= data[31:24];
  	  
		ram[6] <= count_time[7:0];
		ram[7] <= count_time[15:8];
		ram[8] <= count_time[23:16];
		ram[9] <= count_time[31:24];
		
		// next state
		ram[0] <= 8'b00000011; 
	end
	if(ram[0] == 8'b00000011) begin
		data <= 0;
		count_time <= 0;
		ram[0] <= 0;
	end
	
	if(signal == 1 && flag && ram[0] == 8'b00000000) begin
		data <= data + 1;
		flag <= 0;
	end
	if(signal == 0 && ram[0] == 8'b00000000)
		flag <= 1;
	if(ram[0] == 8'b00000000 && !flag)
		count_time <= count_time + 1;
end
endmodule
