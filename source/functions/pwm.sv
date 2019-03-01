//------------------------------------------------------------------------------
// Copyright [2016] [Shchablo Konstantin]

//------------------------------------------------------------------------------
// Information.
// Company: JINR PMTLab
// Author: Shchablo Konstantin
// Email: ShchabloKV@gmail.com
// Tel: 8-906-796-76-53 (russia)
//-------------------------------------------------------------------------------

module pwm
(
	input clk,

	input wire [7:0]addr,
	input wire [7:0]data_in, output wire [7:0]data_out,
	input wire we, input wire init, input wire res,

	input wire we32,
	input wire [31:0]data_in32,
	output reg out

);
reg [7:0]ram[9:0];
reg [31:0]zero_counter;
reg [31:0]signal_counter;
wire [31:0]zero;
wire [31:0]signal;

assign zero[7:0] = ram[2];
assign zero[15:8] = ram[3];
assign zero[23:16] = ram[4];
assign zero[31:24] = ram[5];

assign signal[7:0] = ram[6];
assign signal[15:8] = ram[7];
assign signal[23:16] = ram[8];
assign signal[31:24] = ram[9];


always@(posedge clk)
begin

	if(res || ram[0] == 8'b00000001) begin
		ram[0] <= 0;
		ram[1] <= 0;
		
		ram[2] <= 8'h45;
		ram[3] <= 8'hC3;
		ram[4] <= 0;
		ram[5] <= 0;
		
		ram[6] <= 8'h0A;
		ram[7] <= 8'h00;
		ram[8] <= 0;
		ram[9] <= 0;
		out <= 0;
		zero_counter <= 0;
		signal_counter <= 0;
	end
	
	// initialization
	if(init) begin
		ram[0] <= 0;
		ram[1] <= 0;
		
		ram[2] <= 8'h45;
		ram[3] <= 8'hC3;
		ram[4] <= 0;
		ram[5] <= 0;
		
		ram[6] <= 8'h0A;
		ram[7] <= 8'h00;
		ram[8] <= 0;
		ram[9] <= 0;
		out <= 0;
		zero_counter <= 0;
		signal_counter <= 0;
	end
	
	if(we32) begin
		if(addr == 8'h46) begin
			ram[2] <= data_in32[7:0];
			ram[3] <= data_in32[15:8];
			ram[4] <= data_in32[23:16];
			ram[5] <= data_in32[31:24];			
	  	end
	end
	
	
	  // write memory	
	if(we) begin
		case(addr)
		  8'h36:  ram[0] <= data_in;
		  8'h37:  ram[1] <= data_in;
		  8'h38:  ram[2] <= data_in;
		  8'h39:  ram[3] <= data_in;
	    8'h40:  ram[4] <= data_in;
	    8'h41:  ram[5] <= data_in;
	    8'h42:  ram[6] <= data_in;
	    8'h43:  ram[7] <= data_in;
	    8'h44:  ram[8] <= data_in;
	    8'h45:  ram[9] <= data_in;					
	  	endcase
	end
	case(addr)
	  8'h36:  data_out <= ram[0];
	  8'h37:  data_out <= ram[1];
	  8'h38:  data_out <= ram[2];
	  8'h39:  data_out <= ram[3];
	  8'h40:  data_out <= ram[4];
	  8'h41:  data_out <= ram[5];
	  8'h42:  data_out <= ram[6];
	  8'h43:  data_out <= ram[7];
	  8'h44:  data_out <= ram[8];
	  8'h45:  data_out <= ram[9];
	endcase

	if(zero_counter < zero) begin 
		out <= 1'b0;	
		zero_counter <= zero_counter+32'b1;
	end
	else if(signal_counter < signal && zero_counter == zero) begin
		out <= 1'b1;
		signal_counter <= signal_counter+32'b1;
	end
	else begin		
		out <= 1'b0;
		zero_counter <= 0;
		signal_counter <= 0;
	end
end

endmodule
