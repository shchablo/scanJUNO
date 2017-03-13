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

module vjtag(
  input wire init,
  input wire [7:0]data_in,
  output wire [7:0]data_out,
  output wire [7:0]address,
  output reg we,
  output reg addr_we
  
  
  );

// Signals and registers declared for Virtual JTAG instance
wire tck, tdi;
wire cdr, eldr, e2dr, pdr, sdr, udr, uir, cir, tms;
reg  tdo;
wire [2:0]ir_in; //IR command register

// Instantiation of Virtual JTAG
vjtag_interface vjtag_inst(
.tdo (tdo),
.tck (tck),
.tdi (tdi),
.ir_in(ir_in),
.ir_out(),
.virtual_state_cdr (cdr),
.virtual_state_e1dr(e1dr),
.virtual_state_e2dr(e2dr),
.virtual_state_pdr (pdr),
.virtual_state_sdr (sdr),
.virtual_state_udr (udr),
.virtual_state_uir (uir),
.virtual_state_cir (cir)
);

// INITIALIZATION
reg [7:0]shift_ar_in;
reg [7:0]shift_dr_in;
reg [7:0]shift_dr_out;
assign data_out = shift_dr_in;
assign address = shift_ar_in;
always @(posedge tck) begin

	if(init) begin
		shift_dr_out <= 0;
		shift_dr_in <= 0;
		shift_ar_in <= 0;
		we <= 0;
		addr_we <= 0;
	end
	
	// RESET
	if(address == 8'h01 && data_out == 8'b00000001) begin
		shift_dr_out <= 0;
		shift_dr_in <= 0;
		shift_ar_in <= 0;
		we <= 0;
		addr_we <= 0;
	end
	
  // RECEIVER		
	// ADDRESS receiver
  // shifts incoming address during ADDRESS command
	if(sdr && (ir_in == 3'b001))
		shift_ar_in <= {tdi, shift_ar_in[7:1]};
	// write received address (during ADDRESS command) into register
	if(udr && (ir_in == 3'b001))
		addr_we <= 1;
	else
		addr_we <= 0;
  
  // DATA receiver
  // shifts incoming data during PUSH command 
  if(sdr && (ir_in == 3'b010))
		shift_dr_in <= {tdi, shift_dr_in[7:1]};
	// write signal (during ADDRESS command)
	if(udr && (ir_in == 3'b010))
		we <= 1;
  else
		we <= 0;
		
		// SENDER
		if(cdr && (ir_in == 3'b011))
		// capture data for send during command POP
		shift_dr_out <=  data_in;
  else
		if(sdr && (ir_in == 3'b011))
			// shift out data durng command POP
			shift_dr_out <= {tdi, shift_dr_out[7:1]};
end

// Pass or bypass data via tdo reg.
always @* begin
	case(ir_in)
	 3'b001: tdo <= shift_ar_in [0];
    3'b010: tdo <= shift_dr_in [0];
    3'b011: tdo <= shift_dr_out [0];
	default:
		tdo <= tdi;
  endcase
end

endmodule