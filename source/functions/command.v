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

module command
(
	input	clk, input reset, input init,
	output reg reset_out,
	
	input we_addr_vjtag,
	input [7:0]addr_vjtag,
	
	input we_addr_export,
	input [7:0]addr_export,
	
	output reg [7:0]addr,
	
	input write_vjtag,
	input write_export,
	
	output reg sw_out, 
	
	input [7:0]data_in_export,
	input [7:0]data_in_vjtag,
	
	output reg [7:0]data_out
); 
	(* syn_encoding = "safe" *) reg [4:0] state;

	parameter D0 = 0, R0 = 1, R1 = 2, SW0 = 3, SW1 = 4, vjtag_AD0 = 5, vjtag_AD1 = 6, export_AD0 = 7, export_AD1 = 8, INIT0 = 9, SW0_export = 10, SW1_export = 11;

	// Determine the next state
	integer res_count;
	integer sw_count;
	integer AD_count;
	always @ (posedge clk) begin
			case (state)
				D0: begin
					
					reset_out = 1'b0;
					sw_out  = 1'b0;
				
					 if(!reset || (addr == 8'h01 && data_out == 8'h02))
							state <= R0;
					 if(write_vjtag)
							state <= SW0;
					if(write_export)
							state <= SW0_export;
					 if(we_addr_vjtag)
							state <= vjtag_AD0;
					 if(we_addr_export)
							state <= export_AD0;
					 if(init)
							state <= INIT0;
				end
			 // reset
				R0: begin
					res_count <= 50000;
					state <= R1;
					end
				R1:
					if (res_count > 0) begin
						state <= R1;
						res_count <= res_count - 1;
						
						addr <= 8'h00;
						reset_out = 1'b1;
					end
					else begin
						state <= D0;
					end
				
				// signal write vjtag
				SW0: begin
					sw_count <= 3;
					state <= SW1;
					end
				SW1:
					if (sw_count > 0) begin
						state <= SW1;
						sw_count <= sw_count - 1;
						
						sw_out  = 1'b1;
						data_out <= data_in_vjtag;
					end
					else begin
						state <= D0;
					end
					
					// signal write export
				SW0_export: begin
					sw_count <= 3;
					state <= SW1_export;
					end
				SW1_export:
					if (sw_count > 0) begin
						state <= SW1_export;
						sw_count <= sw_count - 1;
						
						sw_out  = 1'b1;
						data_out <= data_in_export;
					end
					else begin
						state <= D0;
					end
				
				// addr vjtag
				vjtag_AD0: begin
					AD_count <= 3;
					state <= vjtag_AD1;
					end
				vjtag_AD1:
					if (AD_count > 0) begin
						state <= vjtag_AD1;
						AD_count <= AD_count - 1;
						
						addr <= addr_vjtag;
					end
					else begin
						state <= D0;
					end
					
				// addr eth	
				export_AD0: begin
					AD_count <= 50000;
					state <= export_AD1;
					end
				export_AD1:
					if (AD_count > 0) begin
						state <= export_AD1;
						AD_count <= AD_count - 1;
						
						addr <= addr_export;	
					end
					else begin
						state <= D0;
					end
					
				INIT0: begin
					addr <= 0;
					state <= D0;
				end
			
				default:
						state <= D0;
			endcase
	end

endmodule